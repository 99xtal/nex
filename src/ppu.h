#ifndef PPU_H
#define PPU_H

#include <stdint.h>

typedef uint8_t (*ppu2C02_read_fn)(void *ctx, uint16_t address);
typedef void (*ppu2C02_write_fn)(void *ctx, uint16_t address, uint8_t value);

typedef struct ppu2C02 {
    // callbacks for PPU address-space reads/writes: $0000-$3FFF
    ppu2C02_read_fn read;
    ppu2C02_write_fn write;
    void *ctx;
} ppu2C02;

void ppu2C02_init(ppu2C02 *ppu, ppu2C02_read_fn read, ppu2C02_write_fn write, void *ctx);

void ppu2C02_reset(ppu2C02 *ppu);

void ppu2C02_step(ppu2C02 *ppu);

// functions to be used by CPU to read/write to MMIO registers
uint8_t ppu2C02_cpu_read(ppu2C02 *ppu, uint16_t addr);
void ppu2C02_cpu_write(ppu2C02 *ppu, uint16_t addr, uint8_t value);

#endif // PPU_H