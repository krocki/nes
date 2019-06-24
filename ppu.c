#include "ppu.h"
#include "mem.h"
#include "6502.h"
#include <stdio.h>

#define IM_W 256
#define IM_H 240
u8 pix[3*IM_W*IM_H];
u8 vram[16384];
u8 spr_ram[256];

const u16 nt_bases[] = { 0x2000, 0x2400, 0x2800, 0x2c00 };
const u32 palette[] = {
  0x666666, 0x002a88, 0x1412a7, 0x3b00a4,
  0x5c007e, 0x6e0040, 0x6c0600, 0x561d00,
  0x333500, 0x0b4800, 0x005200, 0x004f08,
  0x00404d, 0x000000, 0x000000, 0x000000,
  0xadadad, 0x155fd9, 0x4240ff, 0x7527fe,
  0xa01acc, 0xb71e7b, 0xb53120, 0x994e00,
  0x6b6d00, 0x388700, 0x0c9300, 0x008f32,
  0x007c8d, 0x000000, 0x000000, 0x000000,
  0xfffeff, 0x64b0ff, 0x9290ff, 0xc676ff,
  0xf36aff, 0xfe6ecc, 0xfe8170, 0xea9e22,
  0xbcbe00, 0x88d800, 0x5ce430, 0x45e082,
  0x48cdde, 0x4f4f4f, 0x000000, 0x000000,
  0xfffeff, 0xc0dfff, 0xd3d2ff, 0xe8c8ff,
  0xfbc2ff, 0xfec4ea, 0xfeccc5, 0xf7d8a5,
  0xe4e594, 0xcfef96, 0xbdf4ab, 0xb3f3cc,
  0xb5ebf2, 0xb8b8b8, 0x000000, 0x000000
};

u16 ppu_cycle=340;
u32 scanline=240;
u32 col=0;
u64 frame_cnt=0;
u8 vblank=0;
u8 ppu_regs[8];
u8 nmi=0;
u16 vram_address=0;
u8 oam_address=0;
u8 scroll_reg=0;

u8 write_ppu_regs(u8 a, u8 v) {
  switch (a) {
    case 0 ... 1: ppu_regs[a] = v; break;
    case 3: oam_address = v; break;
    case 4: spr_ram[oam_address] = v; oam_address++; break;
    case 5: scroll_reg = v; break;
    case 6: vram_address = (vram_address << 8) | v; break;
    case 7: vram[vram_address & 0x3fff]=v; u8 inc = bit(PPUCTRL,2) ? 32 : 1; vram_address += inc; break;
    default: printf("error: writing to ppu reg %d %02x\n", a, v);
  }
}

u8 read_ppu_regs(u8 a) {
  u8 v;
  switch (a) {
    case 2:
      v = PPUSTATUS; set(PPUSTATUS,7,0); return v; // clear nmi_occured
    case 4: return spr_ram[oam_address];
    case 7: return vram[vram_address & 0x3fff];
    default: printf("error: reading from ppu reg %d\n", a);
  }
}

void dma_transfer(u8 a) { u16 off=(((u16)a) << 8); for (u16 i=0;i<256;i++) { spr_ram[i] = r8(off+i); } }

void ppu_step(u64 ticks) {

  if (show_debug) {
    //printf("PPUCTRL: "); for (s8 i=7;i>=0;i--) {printf("%d", (PPUCTRL >> i) & 0x1);} printf("\n");
    //printf("PPUSTAT: "); for (s8 i=7;i>=0;i--) {printf("%d", (PPUSTATUS >> i) & 0x1);} printf("\n");
    //printf("nmi = %d\n", nmi);
  }

  u8 nmi_enable = bit(PPUCTRL, 7);
  for (u64 i=0; i<ticks; i++) {

    if (col==256) {col=0; scanline++;}
    if (scanline==240 && col==0) {vblank=1; set(PPUSTATUS,7,1); nmi=nmi_enable;}
    if (scanline==260) {vblank=0; set(PPUSTATUS,7,0); scanline=0; nmi=0;}
    // draw
    u8 px=col; u8 py=scanline;

    if (py<IM_H) {
      //u32 color = colors[(ram[px+(py/8)*IM_W & 0x7ff] >> (py & 7)) & 0x1];
      // render background
      u8 nt_idx = PPUCTRL & 0x3; u16 nt_base = nt_bases[nt_idx];
      u16 bg_table=bit(PPUCTRL,4) ? 0x1000 : 0x0000;
      u16 tile=vram[nt_base + ((py & 248) << 2) + (px >> 3)];
      u16 pattern_off=bg_table + (tile << 4) + (py & 7);
      u8 curattrib = (vram[nt_base + 0x3C0 + ((py & 224) >> 2) + (px >> 5)] >> (((px & 16) >> 3) | ((py & 16) >> 2)) & 3) << 2;
      u8 curpixel = (chr[pattern_off & 0x1fff] >> (~px & 7)) & 1;
      curpixel |= ((chr[(pattern_off+8) & 0x1fff] >> (~px & 7)) & 1) << 1;
      u32 color = palette[vram[0x3f00 + (curattrib | curpixel)] & 0x3f];

			pix[3*(px+py*IM_W)+0] = (color >> 16) & 0xff;
			pix[3*(px+py*IM_W)+1] = (color >> 8) & 0xff;
			pix[3*(px+py*IM_W)+2] = (color >> 0) & 0xff;
    }

    col++; ppu_cycle = (ppu_cycle+1) % 341;

    if (show_debug) {
      //printf("ppu_cycle %d scanline: %d, col %d, vblank %d\n", ppu_cycle, scanline, col, vblank);
    }
  }
 if (nmi) { if (show_debug) /*printf("triggering NMI\n"); */ trigger_nmi(); nmi=0; }
}
