#include "typedefs.h"

#define IM_W 256
#define IM_H 240
#define bit(x,y) (((x) >> (y)) & 0x1)
#define set(x,y,v) { (x) &= ~(1 << (y)); (x) |= ((v) << (y)); }

extern u8 ppu_regs[8];
#define PPUCTRL (ppu_regs[0])
#define PPUSTATUS (ppu_regs[2])

// the screen
extern u8 pix[3*IM_W*IM_H];
extern u8 vram[16384];
extern u8 spr_ram[256];

extern void ppu_step(u64 ticks);
extern u8 read_ppu_regs(u8 a);
extern u8 write_ppu_regs(u8 a, u8 v);
extern void dma_transfer(u8 a);

extern u64 cyc;
extern u64 prev_cyc;
