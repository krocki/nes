#include <stdio.h>
#include <string.h>
#include "typedefs.h"

extern u8 bindata[0x10000];
extern u8 prg[0x4000];
extern u8 chr[0x2000];
extern u8 trainer[0x200];
extern u8 sram[0x2000];
extern u8 ram[0x800];

extern u8 mapper;
extern u8 mirror;
extern u8 battery;

extern void w8(u16 a, u8 v);
extern u8 r8(u16 a);
extern u16 r16_ok(u16 a);
//version with the bug
extern u16 r16(u16 a);

extern void print_mem(u8* addr, u16 rows, s8 mode);
extern u32 readfile(const char* fname, u8* addr, u32 maxsize);
extern void read_ines(const char* fname);
