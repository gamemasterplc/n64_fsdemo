#include <ultra64.h>
#include "fs.h"
#include "cont.h"
#include "gfx.h"
#include "arena.h"

#define TEX_DATA_SIZE 2048

static u8 *bg_tile_seg;

static float scroll_x;
static float scroll_y;
static float zoom;

static void ReadTex()
{
	FSFile file;
	bg_tile_seg = ROUND_PTR_UP(ArenaGetLo(), 8); //Get memory for background tile
	FSOpen(&file, "bg_tile.bin");
	FSRead(&file, bg_tile_seg, TEX_DATA_SIZE);
	FSClose(&file);
	ArenaSetLo(bg_tile_seg+TEX_DATA_SIZE); //Use arena for background tile memory
}

void Stage00Init()
{
	ReadTex();
	scroll_x = scroll_y = 0; //Reset scroll
	zoom = 1.0f; //Original size
}

void Stage00Update()
{
	float dzoom = 0;
	//Zoom with L and R trigger
	if(pad_data[0].button & L_TRIG) {
		dzoom -= 0.01f*zoom;
	}
	if(pad_data[0].button & R_TRIG) {
		dzoom += 0.01f*zoom;
	}
	//Update Zoom
	zoom += dzoom;
	//Clamp Zoom
	if(zoom < 0.25) {
		zoom = 0.25;
	}
	if(zoom > 4) {
		zoom = 4;
	}
	//Scroll with stick
	if(pad_data[0].stick_x > 10 || pad_data[0].stick_x < -10) {
		scroll_x += pad_data[0].stick_x*0.03;
	}
	if(pad_data[0].stick_y > 10 || pad_data[0].stick_y < -10) {
		scroll_y -= pad_data[0].stick_y*0.03;
	}
}

void Stage00Draw()
{
	float tex_x, tex_y;
	static Gfx bg_init[] = {
		//Initialize background render settings
		gsDPSetCycleType(G_CYC_1CYCLE),
		gsDPSetRenderMode(G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF),
		gsDPSetDepthSource(G_ZS_PIXEL),
		gsDPSetTexturePersp(G_TP_NONE),
		gsDPSetPrimColor(0 ,0, 255, 255, 255, 255),
		gsDPSetCombineMode(G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM),
		gsDPSetTextureLUT(G_TT_NONE),
		//Load background texture
		gsDPLoadTextureBlock(0x01000000, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32,
			0, G_TX_WRAP, G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
		gsSPEndDisplayList(),
	};
	GfxClear(0, 0, 0); //Clear screen
	//Draw Background
	gSPSegment(glistp++, 1, bg_tile_seg);
	gSPDisplayList(glistp++, bg_init);
	tex_x = scroll_x-((GFX_FB_W/2)/zoom);
	tex_y = scroll_y-((GFX_FB_W/2)/zoom);
	gSPTextureRectangle(glistp++, 0 << 2, 0 << 2, GFX_FB_W << 2, GFX_FB_H << 2, G_TX_RENDERTILE, tex_x*32, tex_y*32, 1024/zoom, 1024/zoom);
	gDPPipeSync(glistp++);
}