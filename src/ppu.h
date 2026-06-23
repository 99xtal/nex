#ifndef PPU_H
#define PPU_H

#include <stdint.h>

#define SCANLINE_MAX 261
#define DOT_MAX 340

#define VBLANK_SCANLINE 241
#define PRERENDER_SCANLINE 261

typedef uint8_t (*ppu2C02_read_fn)(void *ctx, uint16_t address);
typedef void (*ppu2C02_write_fn)(void *ctx, uint16_t address, uint8_t value);

typedef struct ppu2C02 {
    // PPU state
    int16_t scanline;   // 0-239 visible, 240 post-render, 241-260 vblank, 261 pre-render
    uint16_t dot;       // 0-340, aka PPU cycle within scanline
    uint64_t frame;

    // MMIO registers
    /**
     * bit 7: Vblank flag, cleared on read
     * bit 6: Sprite 0 hit flag
     * bit 5: Sprite overflow flag
     */
    uint8_t status;

    // internal registers
    /**
     * Rendering: used for scroll position
     * Non-rendering: current VRAM addr
     */
    uint8_t v;
    /**
     * Rendering: starting coarse-x scroll for next scanline
     * and starting y scroll for screen
     * Non-rendering: scroll or VRAM addr before transfer to 'v'
     */
    uint8_t t;
    /**
     * Rendering: fine-x position of current scroll
     */
    uint8_t x;
    /**
     * Toggles on each write to either PPUSCROLL or PPUADDR
     * indicating this is first or second write
     * w=0 (first write), w=1 (second write)
     */
    uint8_t w;

    // callbacks for PPU address-space reads/writes: $0000-$3FFF
    ppu2C02_read_fn read;
    ppu2C02_write_fn write;
    void *ctx;
} ppu2C02;

void ppu2C02_init(ppu2C02 *ppu, ppu2C02_read_fn read, ppu2C02_write_fn write, void *ctx);

void ppu2C02_reset(ppu2C02 *ppu);

void ppu2C02_step(ppu2C02 *ppu);

// functions to be used by CPU to read/write to MMIO registers
uint8_t ppu2C02_cpu_read(ppu2C02 *ppu, uint8_t reg);
void ppu2C02_cpu_write(ppu2C02 *ppu, uint8_t reg, uint8_t value);

#endif // PPU_H