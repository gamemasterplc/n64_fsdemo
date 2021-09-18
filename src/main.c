#include <nusys.h>
#include <nualstl.h>
#include "arena.h"
#include "cont.h"
#include "fs.h"
#include "gfx.h"
#include "stage00.h"

#define PTR_BUF_SIZE 8192
#define TUNE_BUF_SIZE 16384

static void AudioInit()
{
	u8 *audio_heap = ROUND_PTR_DOWN(ArenaGetHi()-NU_AU_HEAP_SIZE, 16); //Allocate audio heap
	musConfig config;
    config.control_flag = 0; //Wave data is streamed from ROM
    config.channels = NU_AU_CHANNELS; //Maximum total number of channels
    config.sched = NULL; //The address of the Scheduler structure. NuSystem uses NULL
    config.thread_priority = NU_AU_MGR_THREAD_PRI; //Thread priority (highest)
    config.heap = audio_heap; //Heap address
    config.heap_length = NU_AU_HEAP_SIZE; //Heap size
    config.ptr = NULL; //Allows you to set a default ptr file
    config.wbk = NULL; //Allows you to set a default wbk file
    config.default_fxbank = NULL; //Allows you to set a default bfx file
    config.fifo_length = NU_AU_FIFO_LENGTH; //The size of the library's FIFO buffer
    config.syn_updates = NU_AU_SYN_UPDATE_MAX; //Specifies the number of updates usable by the synthesizer.
    config.syn_output_rate = 44100; //Audio output rate. The higher, the better quality
    config.syn_rsp_cmds = NU_AU_CLIST_LEN; //The maximum length of the audio command list.
    config.syn_retraceCount = 1; //The number of frames per retrace message
    config.syn_num_dma_bufs = NU_AU_DMA_BUFFER_NUM; //Specifies number of buffers Audio Manager can use for DMA from ROM.
    config.syn_dma_buf_size = NU_AU_DMA_BUFFER_SIZE; //The length of each DMA buffer

    //Initialize the Audio Manager.
    nuAuStlMgrInit(&config);
    nuAuPreNMIFuncSet(nuAuPreNMIProc);
	ArenaSetHi(audio_heap); //Make audio heap unusable
}

//Define libmus audio data buffers
static u8 ptr_buf[PTR_BUF_SIZE] __attribute__((aligned(4)));
static u8 tune_buf[TUNE_BUF_SIZE] __attribute__((aligned(4)));

static void MusicStart()
{
	FSFile file;
	FSOpen(&file, "bank_instr.ptr");
	FSRead(&file, ptr_buf, FSGetSize(&file));
	FSClose(&file);
	FSOpen(&file, "bank_instr.wbk");
    MusPtrBankInitialize(ptr_buf, (void *)FSGetRomOfs(&file));
	FSClose(&file);
	FSOpen(&file, "sng_menu.bin");
	FSRead(&file, tune_buf, FSGetSize(&file));
	FSClose(&file);
	MusStartSong(tune_buf);
}

void mainproc()
{
	ArenaInit();
	GfxInit();
	FSInit();
	ContInit();
	AudioInit();
	Stage00Init();
	MusicStart();
	while(1) {
		ContUpdate();
		Stage00Update();
		GfxStartFrame();
		Stage00Draw();
		GfxEndFrame();
	}
}