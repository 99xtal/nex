#include "ppu.h"

typedef enum {
    PPUCTRL_NMI_ENABLE      = (1 << 7),
    PPUCTRL_LF_SELECT       = (1 << 6),
    PPUCTRL_SPRITE_SIZE     = (1 << 5),
    PPUCTRL_BG_PTABLE       = (1 << 4),
    PPUCTRL_SPRITE_PTABLE   = (1 << 3),
    PPUCTRL_VRAM_INC        = (1 << 2),
} ControlFlag;

typedef enum {
    PPUSTATUS_VBLANK             = (1 << 7),
    PPUSTATUS_SPRITE_0_HIT       = (1 << 6),
    PPUSTATUS_SPRITE_OVERFLOW    = (1 << 5)
} StatusFlag;

void ppu2C02_init(ppu2C02 *ppu, ppu2C02_read_fn read, ppu2C02_write_fn write, void *ctx) {
    ppu->scanline = 0;
    ppu->dot = 0;
    ppu->frame = 0;

    ppu->ctrl = 0;
    ppu->status = 0;

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
            ppu->status &= ~(PPUSTATUS_VBLANK | PPUSTATUS_SPRITE_OVERFLOW | PPUSTATUS_SPRITE_0_HIT);
            ppu->nmi_pending = 0;
        }
    }

    // Vblank phase
    if (ppu->scanline == VBLANK_SCANLINE && ppu->dot == 1) {
        ppu->status |= PPUSTATUS_VBLANK;
        if (ppu->ctrl & PPUCTRL_NMI_ENABLE) {
            ppu->nmi_pending = 1;
        }
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
            // PPUSTATUS
            ppu->w = 0;
            return ppu->status;
        }
        default:
            return 0;
    }
}

void ppu2C02_cpu_write(ppu2C02 *ppu, uint8_t reg, uint8_t value) {
    switch (reg) {
        case 0: {
            // PPUCTRL
            // setting "NMI enabled" while Vblank flag is set
            // should trigger an NMI
            int nmi_was_enabled = ppu->ctrl & PPUCTRL_NMI_ENABLE;
            ppu->ctrl = value;
            int nmi_now_enabled = ppu->ctrl & PPUCTRL_NMI_ENABLE;

            if (!nmi_was_enabled && nmi_now_enabled && (ppu->status & PPUSTATUS_VBLANK)) {
                ppu->nmi_pending = 1;
            }
            
            break;
        }
        default:
            break;
    }
}