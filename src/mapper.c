#include "cartridge.h"
#include "mapper.h"

uint8_t nrom_cpu_read(mapper *m, uint16_t addr) {
    cartridge *c = m->ctx;

    if (addr < 0x8000) {
        return 0;
    }

    if (c->prg_rom_size == 0x4000) {
        // NROM-128: 16 KB mirrored at $8000-$BFFF and $C000-$FFFF
        return c->prg_rom[(addr - 0x8000) % 0x4000];
    }

    // NROM-256: 32 KB mapped directly at $8000-$FFFF
    return c->prg_rom[addr - 0x8000];
}

void nrom_cpu_write(mapper *m __attribute__((unused)), uint16_t addr __attribute__((unused)), uint8_t value __attribute__((unused))) {}

uint8_t nrom_ppu_read(mapper *m, uint16_t addr) {
    cartridge *c = m->ctx;

    if (addr < 0x2000) {
        return c->chr_rom[addr];
    }

    return 0;
}

void nrom_ppu_write(mapper *m, u_int16_t addr, uint8_t value) {
    cartridge *c = m->ctx;

    if (addr < 0x2000 && c->uses_chr_ram) {
        c->chr_rom[addr] = value;
    }
}

void mapper_nrom_init(mapper *m, cartridge *c) {
    m->ctx = c;
    m->cpu_read = nrom_cpu_read;
    m->cpu_write = nrom_cpu_write;
    m->ppu_read = nrom_ppu_read;
    m->ppu_write = nrom_ppu_write;
}