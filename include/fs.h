#pragma once

#include <ultra64.h>
#include "bool.h"

#define FS_SEEK_SET 0
#define FS_SEEK_CUR 1
#define FS_SEEK_END 2

#define FS_INVALID_FILE_ID -1

typedef struct fs_file {
	s32 pos;
	u32 rom_ofs;
	s32 size;
} FSFile;

void FSInit();
s32 FSGetFileId(char *path);
bool FSOpen(FSFile *file, char *path);
bool FSOpenFast(FSFile *file, s32 file_id);
s32 FSRead(FSFile *file, void *dst, s32 size);
u32 FSGetRomOfs(FSFile *file);
s32 FSGetRomSize(FSFile *file);
s32 FSGetSize(FSFile *file);
bool FSSeek(FSFile *file, s32 offset, u32 origin);
s32 FSTell(FSFile *file);

#define FSClose(file) //Empty implementation for FSClose