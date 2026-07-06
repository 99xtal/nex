#include "mapper.h"

#include "cartridge.h"

uint8_t nrom_cpu_read(Mapper* m, uint16_t addr) {
  Cartridge* c = m->ctx;

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

void nrom_cpu_write(Mapper* m, uint16_t addr, uint8_t value) {
  Cartridge* c = m->ctx;

  if (addr >= 0x6000 && addr < 0x8000) {
    c->prg_ram[(addr - 0x6000) % 0x2000] = value;
  }
}

uint8_t nrom_ppu_read(Mapper* m, uint16_t addr) {
  Cartridge* c = m->ctx;

  if (addr < 0x2000) {
    return c->chr_rom[addr];
  }

  return 0;
}

void nrom_ppu_write(Mapper* m, u_int16_t addr, uint8_t value) {
  Cartridge* c = m->ctx;

  if (addr < 0x2000 && c->uses_chr_ram) {
    c->chr_rom[addr] = value;
  }
}

void mapper_nrom_init(Mapper* m, Cartridge* c) {
  m->ctx = c;
  m->cpu_read = nrom_cpu_read;
  m->cpu_write = nrom_cpu_write;
  m->ppu_read = nrom_ppu_read;
  m->ppu_write = nrom_ppu_write;
}

/**
 * Mapper 3: CNROM
 */

uint8_t cnrom_cpu_read(Mapper* m, uint16_t addr) {
  Cartridge* c = m->ctx;

  if (addr < 0x6000) {
    return 0;
  }

  if (addr >= 0x6000 && addr < 0x8000) {
    return c->prg_ram[(addr - 0x6000) % 0x800];
  }

  return c->prg_rom[addr - 0x8000];
}

void cnrom_cpu_write(Mapper* m, uint16_t addr, uint8_t value) {
  Cartridge* c = m->ctx;
  MapperCNROMState* s = m->state;

  if (addr >= 0x8000) {
    s->chr_bank = value & 0x03;
  };
}

uint8_t cnrom_ppu_read(Mapper* m, uint16_t addr) {
  Cartridge* c = m->ctx;
  MapperCNROMState* s = m->state;

  if (addr < 0x2000) {
    size_t index = ((size_t)s->chr_bank * 0x2000) + addr;
    return c->chr_rom[index % c->chr_rom_size];
  }

  return 0;
}

void cnrom_ppu_write(Mapper* m, uint16_t addr, uint8_t value) {
  (void)m;
  (void)addr;
}

int mapper_cnrom_init(Mapper* m, Cartridge* c) {
  MapperCNROMState* s = calloc(1, sizeof(*s));
  if (!s) return -1;

  m->ctx = c;
  m->state = s;
  m->cpu_read = cnrom_cpu_read;
  m->cpu_write = cnrom_cpu_write;
  m->ppu_read = cnrom_ppu_read;
  m->ppu_write = cnrom_ppu_write;

  return 0;
}