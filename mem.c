#include "mem.h"
#include "ppu.h"

// for reading in ines
u8 bindata[0x10000];
u8 ram[0x800];
u8 prg[0x4000];
u8 chr[0x2000];
u8 trainer[0x200];
u8 sram[0x2000];

u8 mapper;
u8 mirror;
u8 battery;

//cpu space funcs
void w8(u16 a, u8 v) {
  switch (a) {
    case 0x0000 ... 0x1fff: ram[a & 0x7ff] = v; break;
    case 0x2000 ... 0x3fff: write_ppu_regs(a & 0x7,v); break;
    case 0x4014: dma_transfer(v); break; // dma
    case 0x4016: break; // joypad strobe
    case 0x6000 ... 0xffff: break;
  }
}

u8 r8(u16 a) {
  switch (a) {
    case 0x0000 ... 0x1fff: return ram[a & 0x7ff];
    case 0x2000 ... 0x3fff: return read_ppu_regs(a & 0x7);
    case 0x4014: break; // dma
    case 0x4016: break; // joypad 0
    case 0x4017: break; // joypad 1
    case 0x6000 ... 0x7fff: break;
    case 0x8000 ... 0xffff: return prg[a & 0x3fff];
  }
}

u16 r16_ok(u16 a) { return (r8(a) | (r8(a+1) << 8)); }
//version with the bug
u16 r16(u16 a) { u16 base=a & 0xff00; return (r8(a) | (r8(base|((u8)(a+1))) << 8)); }

//void print_mem(uint16_t off, uint16_t n) {
//
//  uint8_t col_count = 16;
//  for (uint8_t j = 0; j < n; j++) {
//    printf("0x%04X:", off);
//    for (uint8_t i = 0; i < col_count; i++) {
//      printf(" 0x%02x", r8(off));
//      if (i==(col_count-1)) printf("\n");
//      off++;
//    }
//  }
//}
void print_mem(u8* a, u16 n, s8 mode) {
  u16 off=0;
  uint8_t col_count = mode == 0 ? 16 : 64;
  for (uint8_t j = 0; j < n; j++) {
    printf("0x%04X:", off);
    for (uint8_t i = 0; i < col_count; i++) {
      switch (mode) {
        case 0: printf(" 0x%02x", a[off]); break;
        case 1: printf("%c", a[off]); break;
      }
      if (i==(col_count-1)) printf("\n");
      off++;
    }
  }
}

u32 readfile(const char* fname, u8* mem, u32 ramsize) {

  FILE * file = fopen(fname, "r+");
  if (file == NULL) return 0;
  fseek(file, 0, SEEK_END);
  long int size = ftell(file);
  fclose(file);
  //printf("%s, size=%ld\n", fname, size);

  u32 bytes_read = 0;
  // Reading data to array of unsigned chars
  if (ramsize >= size) {
    file = fopen(fname, "r+");
    bytes_read = fread(mem, sizeof(u8), size, file);
    fclose(file);
  }
  return bytes_read;
}

void read_ines(const char* args) {

  if (!readfile((const char*)args, bindata, 0x100000)) { printf("couldn't read %s\n", args); } else
  {
    // ines
    u32 ines = ((u32*)bindata)[0]; printf("checking header: 0x%08x ", ines);
    if (ines != 0x1a53454e) printf(": NOT an ines format!\n");
    else {
      printf(": OK\n");
      u8 num_prg_banks = bindata[4]; u8 num_chr_banks = bindata[5];
      u8 ctrl0 = bindata[6]; u8 ctrl1 = bindata[7];
      mapper = (ctrl0 >> 4 & 0x0f) | (ctrl1 & 0xf0);
      mirror = (ctrl0 & 0x1) | (((ctrl0 >> 3) & 0x1) << 1);
      battery = (ctrl0 >> 1) & 0x1;
      u8 prg_ram_size = bindata[8];
      // 9-15 usused
      printf("mapper: %d, mirror: %d, battery: %d, "
             "%d prg banks(s), %d chr bank(s), %d x 8kB RAM\n",
             mapper, mirror, battery,
             num_prg_banks, num_chr_banks, prg_ram_size
            );
      u32 off=16;
      if ((ctrl0>>2) & 0x1) { //trainer
        memcpy(trainer, bindata+off, 0x200); off+=512;
      }
      memcpy(prg, bindata+off, 0x4000); off+=0x4000; //16kB
      memcpy(chr, bindata+off, 0x2000); off+=0x2000; //8kB
      printf("prg\n"); print_mem(prg, 1, 1);
      printf("chr\n"); print_mem(chr, 1, 0);
    }
  }
}
