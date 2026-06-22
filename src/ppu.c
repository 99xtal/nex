#include "ppu.h"

void ppu2C02_init(ppu2C02 *ppu, ppu2C02_read_fn read, ppu2C02_write_fn write, void *ctx) {
    ppu->read = read;
    ppu->write = write;
    ppu->ctx = ctx;
    return;
}

void ppu2C02_reset(ppu2C02 *ppu) {
    return;
}

void ppu2C02_step(ppu2C02 *ppu) {
    return;
}

uint8_t ppu2C02_cpu_read(ppu2C02 *ppu, uint16_t addr) {
    return 0;
}

void ppu2C02_cpu_write(ppu2C02 *ppu, uint16_t addr, uint8_t value) {
    return;
}