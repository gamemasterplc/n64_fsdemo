#include <ultra64.h>

extern u8 __ArenaLo[];

static void *arena_lo = (void *)0x80000000;
static void *arena_hi = (void *)0x80400000;

void ArenaInit()
{
	arena_lo = __ArenaLo;
	arena_hi = OS_PHYSICAL_TO_K0(osMemSize);
}

void *ArenaGetLo()
{
	return arena_lo;
}

void *ArenaGetHi()
{
	return arena_hi;
}

void ArenaSetLo(void *new_lo)
{
	arena_lo = new_lo;
}

void ArenaSetHi(void *new_hi)
{
	arena_hi = new_hi;
}