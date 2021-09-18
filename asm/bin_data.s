# assembler directives
.set noat      # allow manual use of $at
.set noreorder # don't insert nops after branches
.set gp=64

.include "macros.inc"

.section .data

.balign 2
glabel fsheader_start
.incbin "filesystem.fsh"
.balign 2
glabel fsheader_end

glabel fsdata_start
.incbin "filesystem.bin"
.balign 2
glabel fsdata_end
