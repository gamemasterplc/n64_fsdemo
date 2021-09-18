#include <ultra64.h>
#include "arena.h"
#include "fs.h"

extern u8 fsheader_start[];
extern u8 fsheader_end[];
extern u8 fsdata_start[];

#define FS_DMA_CHUNK_SIZE 1024

typedef struct fs_entry_t {
	char *path;
	u32 rom_ofs;
    u32 size;
} FSEntry;

typedef struct fs_header_t {
	u32 file_count;
	FSEntry entries[1];
} FSHeader;

static FSHeader *header;
static u8 dma_buf[FS_DMA_CHUNK_SIZE] __attribute__((aligned(16))); //16-byte aligned buffer for unaligned reads

static OSPiHandle *GetCartHandle()
{
	static OSPiHandle *handle = NULL;
	//Only acquire handle if necessary
	if(!handle) {
		handle = osCartRomInit();
	}
	return handle;
}

static void ReadRom(void *dst, u32 src, u32 len)
{
	OSIoMesg io_mesg;
    OSMesgQueue dma_msg_queue;
    OSMesg dma_msg;
	char *dst_ptr = dst; //Use temporary for destination pointer
	//Initialize DMA Status
	osCreateMesgQueue(&dma_msg_queue, &dma_msg, 1);
	io_mesg.hdr.pri = OS_MESG_PRI_NORMAL;
    io_mesg.hdr.retQueue = &dma_msg_queue;
	//Check for direct DMA being possible
	if((((u32)dst_ptr & 0x7) == 0) && (src & 0x1) == 0 && (len & 0x1) == 0) {
		//Do DMA directly to destination
		if(((u32)dst_ptr & 0xF) == 0 && (len & 0xF) == 0) {
			//Can skip writeback if 16-byte aligned
			osInvalDCache(dst, len);
		} else {
			//Cannot skip writeback
			osWritebackDCache(dst, (len+15) & ~0xF);
			osInvalDCache(dst, (len+15) & ~0xF);
		}
		while(len) {
			//Copy up to FS_DMA_CHUNK_SIZE bytes or
			u32 copy_len = FS_DMA_CHUNK_SIZE;
			if(copy_len > len) {
				copy_len = len;
			}
			//Setup DMA params
			io_mesg.dramAddr = dst_ptr;
			io_mesg.devAddr = src;
			io_mesg.size = copy_len;
			//Start reading from ROM
			osEPiStartDma(GetCartHandle(), &io_mesg, OS_READ);
			//Wait for ROM read to finish
			osRecvMesg(&dma_msg_queue, &dma_msg, OS_MESG_BLOCK);
			//Advance data pointers
			src += copy_len;
			dst_ptr += copy_len;
			len -= copy_len;
		}
	} else {
		u32 dma_buf_offset = src & 0x1; //Source fixup offset for odd source offset DMAs
		src &= ~0x1; //Round down source offset
		//Writeback invalidate destination buffer for RCP usage
		osWritebackDCache(dst, (len+15) & ~0xF);
		osInvalDCache(dst, (len+15) & ~0xF);
		//DMA to temporary buffer
		while(len) {
			u32 copy_len = FS_DMA_CHUNK_SIZE;
			if(copy_len > len) {
				copy_len = len;
			}
			u32 read_len = (copy_len+15) & ~0xF; //Round read length to nearest cache line
			//Simple invalidate works here since the buffer is aligned to 16 bytes
			osInvalDCache(dma_buf, read_len);
			//Setup DMA params
			io_mesg.dramAddr = dma_buf;
			io_mesg.devAddr = src;
			io_mesg.size = read_len;
			//Start reading from ROM
			osEPiStartDma(GetCartHandle(), &io_mesg, OS_READ);
			//Wait for ROM read to finish
			osRecvMesg(&dma_msg_queue, &dma_msg, OS_MESG_BLOCK);
			//Copy from temporary buffer
			bcopy(dma_buf+dma_buf_offset, dst_ptr, copy_len);
			//Advance data pointers
			src += copy_len;
			dst_ptr += copy_len;
			len -= copy_len;
		}
	}
}

static void FixupHeader()
{
	for(u32 i=0; i<header->file_count; i++) {
		//Name changed to direct RAM pointers
		header->entries[i].path = (char *)((u32)header->entries[i].path+(u32)header);
		//Calculate accurate ROM offset
		header->entries[i].rom_ofs += (u32)fsdata_start;
	}
}

void FSInit()
{
	//Header starts in arena
	header = ROUND_PTR_UP(ArenaGetLo(), __alignof__(FSHeader));
	ReadRom(header, (u32)fsheader_start, fsheader_end-fsheader_start);
	FixupHeader();
	//Round arena to 16 bytes
	ArenaSetLo(ROUND_PTR_UP((char *)header+(fsheader_end-fsheader_start), 16));
}

static s32 namecmp(char *name1, char *name2)
{
	while(*name1) {
		if(*name1 != *name2) {
			break;
		}
		name1++;
		name2++;
	}
	return (*name1)-(*name2);
	
}

s32 FSGetFileID(char *path)
{
	if(*path == '/') {
		path++;
	}
	for(u32 i=0; i<header->file_count; i++) {
		if(namecmp(path, header->entries[i].path) == 0) {
			return i;
		}
	}
	return FS_INVALID_FILE_ID;
}

bool FSOpen(FSFile *file, char *path)
{
	return FSOpenFast(file, FSGetFileID(path));
}


bool FSOpenFast(FSFile *file, s32 file_id)
{
	//Fail if invalid file ID
	if(file_id == FS_INVALID_FILE_ID) {
		return false;
	}
	//Initialize file structure
	file->pos = 0;
	file->rom_ofs = header->entries[file_id].rom_ofs;
	file->size = header->entries[file_id].size;
	return true;
}

s32 FSRead(FSFile *file, void *dst, s32 len)
{
	//Clamp read to end of file
	if(file->pos+len > file->size) {
		len = file->size-file->pos;
	}
	//No-op if no bytes to read
	if(len > 0) {
		ReadRom(dst, file->rom_ofs+file->pos, len);
		//Update file read offset
		file->pos += len;
	}
	return len;
}

u32 FSGetRomOfs(FSFile *file)
{
	return file->rom_ofs;
}

s32 FSGetRomSize(FSFile *file)
{
	//Rounded to next multiple of 2 bytes
	return (file->size+1) & ~0x1;
}

s32 FSGetSize(FSFile *file)
{
	return file->size;
}

bool FSSeek(FSFile *file, s32 offset, u32 origin)
{
	switch(origin) {
		case FS_SEEK_SET:
		//Direct offset
			file->pos = offset;
			break;
			
		case FS_SEEK_CUR:
		//Relative offset
			file->pos += offset;
			break;
			
		case FS_SEEK_END:
		//End-based offset
			file->pos = file->size+offset;
			break;
			
		default:
		//Seek failed
			return false;
	}
	//Clamp seek to file boundaries
	if(file->pos < 0) {
		file->pos = 0;
	}
	if(file->pos > file->size) {
		file->pos = file->size;
	}
	return true;
}

s32 FSTell(FSFile *file)
{
	return file->pos;
}