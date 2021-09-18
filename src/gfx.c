#include <nusys.h>
#include "arena.h"
#include "gfx.h"

static void AllocZBuffer()
{
	u8 *zbuf = ArenaGetLo(); //Grab z-buffer space from start of arena
	//Round Z-Buffer pointer to 64-byte boundary
	zbuf = ROUND_PTR_UP(zbuf, 64);
	ArenaSetLo(zbuf+GFX_FB_SIZE); //Make z-buffer arena space unusable
	nuGfxSetZBuffer((u16 *)zbuf); //Make nusys recognize z-buffer
}

static void AllocCFB()
{
	//CFB Pointer table
	//Made static so it would be persistent in RAM
	static u8 *cfb_list[GFX_FB_COUNT];
	u8 *cfb_base = ArenaGetHi(); //Grab CFB space from end of arena
	cfb_base -= GFX_FB_SIZE*GFX_FB_COUNT;
	cfb_base = ROUND_PTR_DOWN(cfb_base, 64); //Round CFB base to 64 bytes
	//Setup CFB pointers
	for(int i=0; i<GFX_FB_COUNT; i++) {
		cfb_list[i] = cfb_base+(GFX_FB_SIZE*i);
	}
	ArenaSetHi(cfb_base); //Make CFB arena space unusable
	nuGfxSetCfb((u16 **)cfb_list, GFX_FB_COUNT); //Make nusys recognize CFB list
}

static void PatchVIMode(OSViMode *mode, int width, int height)
{
	//Set VI width and x scale
	mode->comRegs.width = width;
	mode->comRegs.xScale = (width*512)/320;
	if(height > 240) {
		mode->comRegs.ctrl |= 0x40; //Forces Serrate Bit On
		//Offset even fields by 1 line and odd fields by 2 lines
		mode->fldRegs[0].origin = width*2;
		mode->fldRegs[1].origin = width*4;
		//Calculate Y scale and offset by half a pixel
		mode->fldRegs[0].yScale = 0x2000000|((height*1024)/240);
		mode->fldRegs[1].yScale = 0x2000000|((height*1024)/240);
		//Fix VI start for even fields
		mode->fldRegs[0].vStart = mode->fldRegs[1].vStart-0x20002;
	} else {
		//Progressive scan mode
		//Offset fields by 1 line
		mode->fldRegs[0].origin = width*2;
		mode->fldRegs[1].origin = width*2;
		//Calculate Y scale
		mode->fldRegs[0].yScale = ((height*1024)/240);
		mode->fldRegs[1].yScale = ((height*1024)/240);
	}
}

static void InitVI()
{
	static OSViMode mode;
	//Copy appropriate mode depending on TV type
	switch(osTvType) {
		case 0:
			mode = osViModeFpalLpn1;
			break;

		case 1:
			mode = osViModeNtscLpn1;
			break;

		case 2:
			mode = osViModeMpalLpn1;
			break;

		default:
			return;
	}
	//Patch and setup VI mode
	PatchVIMode(&mode, GFX_FB_W, GFX_FB_H);
	osViSetMode(&mode);
	//Disable Gamma
	osViSetSpecialFeatures(OS_VI_GAMMA_OFF);
	//Scale VI vertically on PAL
	if(osTvType == 0) {
		osViSetYScale(0.833);
	}
}

static void PreNMICB()
{
	//Prevent Reset Crash
	nuGfxDisplayOff();
	osViSetYScale(1);
}

void GfxInit()
{
	nuGfxInit();
	AllocZBuffer();
	AllocCFB();
	InitVI();
	nuPreNMIFuncSet(PreNMICB);
	nuGfxDisplayOn();
}

static Vp viewport = {
	GFX_FB_W * 2, GFX_FB_H * 2, G_MAXZ / 2, 0,
	GFX_FB_W * 2, GFX_FB_H * 2, G_MAXZ / 2, 0,
};

static Gfx rspinit_dl[] = {
	gsSPViewport(&viewport),
	gsSPClearGeometryMode(G_SHADE | G_SHADING_SMOOTH | G_CULL_BOTH |
							G_FOG | G_LIGHTING | G_TEXTURE_GEN |
							G_TEXTURE_GEN_LINEAR | G_LOD),
	gsSPTexture(0, 0, 0, 0, G_OFF),
	gsSPEndDisplayList(),
};

static Gfx rdpinit_dl[] = {
	gsDPSetCycleType(G_CYC_1CYCLE),
	gsDPPipelineMode(G_PM_1PRIMITIVE),
	gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, GFX_FB_W, GFX_FB_H),
	gsDPSetCombineMode(G_CC_PRIMITIVE, G_CC_PRIMITIVE),
	gsDPSetCombineKey(G_CK_NONE),
	gsDPSetAlphaCompare(G_AC_NONE),
	gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
	gsDPSetColorDither(G_CD_DISABLE),
	gsDPSetAlphaDither(G_AD_DISABLE),
	gsDPSetTextureFilter(G_TF_POINT),
	gsDPSetTextureConvert(G_TC_FILT),
	gsDPSetTexturePersp(G_TP_NONE),
	gsDPPipeSync(),
	gsSPEndDisplayList(),
};

Gfx *glistp;
static Gfx glist[GFX_GLIST_SIZE];

void GfxStartFrame()
{
	glistp = glist;
	gSPSegment(glistp++, 0, 0); //Setup segment 0 as direct segment
	//Call initialization display lists
	gSPDisplayList(glistp++, rspinit_dl);
	gSPDisplayList(glistp++, rdpinit_dl);
}

void GfxClear(u8 r, u8 g, u8 b)
{
	gDPPipeSync(glistp++);
	//Clear Z buffer
	gDPSetCycleType(glistp++, G_CYC_FILL);
	gDPSetDepthImage(glistp++, nuGfxZBuffer);
	gDPSetColorImage(glistp++, G_IM_FMT_RGBA, G_IM_SIZ_16b, GFX_FB_W, nuGfxZBuffer);
	gDPSetFillColor(glistp++, (GPACK_ZDZ(G_MAXFBZ, 0) << 16 | GPACK_ZDZ(G_MAXFBZ, 0)));
	gDPFillRectangle(glistp++, 0, 0, GFX_FB_W - 1, GFX_FB_H - 1);
	gDPPipeSync(glistp++);
	//Clear framebuffer
	gDPSetCycleType(glistp++, G_CYC_FILL);
	gDPSetColorImage(glistp++, G_IM_FMT_RGBA, G_IM_SIZ_16b, GFX_FB_W, nuGfxCfb_ptr);
	gDPSetFillColor(glistp++, (GPACK_RGBA5551(r, g, b, 1) << 16 | GPACK_RGBA5551(r, g, b, 1)));
	gDPFillRectangle(glistp++, 0, 0, GFX_FB_W - 1, GFX_FB_H - 1);
	gDPPipeSync(glistp++);
}

void GfxEndFrame()
{
	//End display list
	gDPFullSync(glistp++);
	gSPEndDisplayList(glistp++);
	//Send display list to RSP
	nuGfxTaskStart(glist, (s32)(glistp - glist) * sizeof(Gfx), NU_GFX_UCODE_F3DEX2, NU_SC_SWAPBUFFER);
	//Wait for frame end
	nuGfxRetraceWait(1);
	nuGfxTaskAllEndWait();
}