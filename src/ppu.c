#include "ppu.h"

typedef enum {
    FLAG_VBLANK             = (1 << 7),
    FLAG_SPRITE_0_HIT       = (1 << 6),
    FLAG_SPRITE_OVERFLOW    = (1 << 5)
} StatusFlag;

void ppu2C02_init(ppu2C02 *ppu, ppu2C02_read_fn read, ppu2C02_write_fn write, void *ctx) {
    ppu->scanline = 0;
    ppu->dot = 0;
    ppu->frame = 0;

    ppu->read = read;
    ppu->write = write;
    ppu->ctx = ctx;
    return;
}

void ppu2C02_reset(ppu2C02 *ppu) {
    return;
}

void ppu2C02_step(ppu2C02 *ppu) {
    // Pre-render phase
    if (ppu->scanline == PRERENDER_SCANLINE) {
        if (ppu->dot == 1) {
            // clear status flags
            ppu->status &= ~(FLAG_VBLANK | FLAG_SPRITE_OVERFLOW | FLAG_SPRITE_0_HIT);
        }
    }

    // Vblank phase
    if (ppu->scanline == VBLANK_SCANLINE && ppu->dot == 1) {
        ppu->status |= FLAG_VBLANK;
    }

    // increment state
    ppu->dot++;
    if (ppu->dot > DOT_MAX) {
        ppu->dot = 0;
        ppu->scanline++;
        if (ppu->scanline > SCANLINE_MAX) {
            ppu->scanline = 0;
            ppu->frame++;
        }
    }
}

uint8_t ppu2C02_cpu_read(ppu2C02 *ppu, uint8_t reg) {
    switch (reg) {
        case 2: {
            ppu->w = 0;
            return ppu->status;
        }
        default:
            return 0;
    }
}

void ppu2C02_cpu_write(ppu2C02 *ppu, uint8_t reg, uint8_t value) {
    return;
}