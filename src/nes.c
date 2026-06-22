#include <string.h>
#include "nes.h"

uint8_t nes_cpu_read(void *ctx, uint16_t addr) {
    nes *n = ctx;

    if (addr < 0x2000) {
        return n->internal_ram[addr % 0x0800];
    }

    if (addr >= 0x2000 && addr < 0x4000) {
        // TODO: PPU registers
        return 0;
    }

    if (addr >= 0x4000 && addr < 0x4018) {
        // TODO: APU/IO registers
        return 0;
    }

    if (addr >= 0x4020) {
        return n->cartridge->mapper.cpu_read(&n->cartridge->mapper, addr);
    }

    return 0;
}

void nes_cpu_write(void *ctx, uint16_t addr, uint8_t value) {
    nes *n = ctx;

    if (addr < 0x2000) {
        n->internal_ram[addr % 0x0800] = value;
    }

    if (addr >= 0x2000 && addr < 0x4000) {
        // TODO: PPU registers
        return;
    }

    if (addr >= 0x4000 && addr < 0x4018) {
        // TODO: APU/IO registers
        return;
    }

    if (addr >= 0x4020) {
        n->cartridge->mapper.cpu_write(&n->cartridge->mapper, addr, value);
    }

    return;
}

int nes_init(nes *n, cartridge *c) {
    if (!n || !c) {
        return -1;
    }

    memset(n, 0, sizeof(*n));
    n->total_cpu_cycles = 0;
    n->cartridge = c;

    cpu6502_init(&n->cpu, nes_cpu_read, nes_cpu_write, n);

    return 0;
}

void nes_reset(nes *n) {
    if (!n) {
        return;
    }

    cpu6502_reset(&n->cpu);
}

void nes_step(nes *n) {
    int cpu_cycles = cpu6502_step(&n->cpu);
    n->total_cpu_cycles += cpu_cycles;
}

void nes_free(nes *n) {
    if (!n) {
        return;
    }

    free(n);
}