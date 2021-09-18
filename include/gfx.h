#pragma once

#include <ultra64.h>

#define GFX_GLIST_SIZE 2048

#define GFX_FB_W 424
#define GFX_FB_H 240
#define GFX_FB_COUNT 3
#define GFX_FB_SIZE (((GFX_FB_W*GFX_FB_H*2)+63) & ~0x3F)

void GfxInit();
void GfxStartFrame();
void GfxClear(u8 r, u8 g, u8 b);
void GfxEndFrame();

extern Gfx *glistp;