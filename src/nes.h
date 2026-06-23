#ifndef NES_H
#define NES_H

#include <lib6502/6502.h>
#include "cartridge.h"
#include "ppu.h"

typedef struct {
    cpu6502 cpu;
    uint8_t wram[0x0800];   // 2KB work RAM for CPU

    ppu2C02 ppu;
    uint8_t vram[0x0800];   // 2KB video RAM for PPU

    cartridge *cartridge;

    uint64_t total_cpu_cycles;
} nes;

int nes_init(nes *n, cartridge *c);

void nes_reset(nes *n);

void nes_step(nes *n);

void nes_free(nes *n);

#endif // NES_H