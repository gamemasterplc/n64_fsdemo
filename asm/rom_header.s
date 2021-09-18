/*
 * ROM header
 * Only the first 0x18 bytes matter to the console.
 */

.byte  0x80, 0x37, 0x12, 0x40 /* PI BSD Domain 1 register */
.word  0x0000000F  /* Clockrate setting*/
.word  __start /* Entrypoint */
.word  0x0000144C /* SDK Revision */
.word  0x00000000 /* Checksum 1 (OVERWRITTEN BY MAKEMASK)*/
.word  0x00000000 /* Checksum 2 (OVERWRITTEN BY MAKEMASK)*/
.word  0x00000000 /* Unknown */
.word  0x00000000 /* Unknown */
.ascii "No Gfxfunc Nusys    " /* Internal ROM name (Exactly 20 characters) */
.byte 0, 0, 0, 0, 0, 0, 0 /* Padding */
.ascii "N" /* Media Type (Cartridge) */
.ascii "PL" /* Cartridge ID (PL)*/
.ascii "E" /* Region (E)*/
.byte  0x00 /* Version */
