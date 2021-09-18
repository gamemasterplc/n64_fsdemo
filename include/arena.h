#pragma once

void ArenaInit();
void *ArenaGetLo();
void *ArenaGetHi();
void ArenaSetLo(void *new_lo);
void ArenaSetHi(void *new_hi);

#define ROUND_PTR_UP(ptr, alignment) ((void *)(((u32)(ptr)+(alignment)-1) & ~((alignment)-1)))
#define ROUND_PTR_DOWN(ptr, alignment) (void *)(((u32)(ptr) & ~((alignment)-1)))