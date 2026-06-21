#ifndef NES_H
#define NES_H

#include <lib6502/6502.h>
#include "cartridge.h"

typedef struct {
    uint8_t internal_ram[0x0800];
    cpu6502 cpu;
    cartridge *cartridge;
} nes;

int nes_init(nes *n, cartridge *c);

void nes_reset(nes *n);

void nes_step(nes *n);

void nes_free(nes *n);

#endif // NES_H