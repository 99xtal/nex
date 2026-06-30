#ifndef NES_H
#define NES_H

#include <lib6502/6502.h>
#include <nex/nes.h>

#include "cartridge.h"
#include "ppu.h"

struct nes {
  cpu6502 cpu;
  uint8_t wram[0x0800];  // 2KB work RAM for CPU

  ppu2C02 ppu;
  uint8_t vram[0x0800];  // 2KB video RAM for PPU

  cartridge* cartridge;

  uint64_t total_cpu_cycles;
};

#endif  // NES_H