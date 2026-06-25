#include <stdbool.h>
#include <string.h>

#include "ppu.h"

#define VRAM_START_ADDR 0x2000
#define NT_SIZE 0x03C0 // size of nametable ()

void update_bg_fetch_pipeline(ppu2C02 *ppu);
void increment_scanline_position(ppu2C02 *ppu);
bool rendering_enabled(ppu2C02 *ppu);
uint16_t get_current_nametable_address(ppu2C02 *ppu);
uint8_t get_bg_palette_number(ppu2C02 *ppu);
uint16_t get_current_attribute_address(ppu2C02 *ppu);
uint16_t get_bg_pattern_address(ppu2C02 *ppu, uint16_t tile_num);
uint16_t coarse_x(ppu2C02 *ppu);
uint16_t coarse_y(ppu2C02 *ppu);
uint16_t fine_y(ppu2C02 *ppu);

typedef enum {
    PPUCTRL_NMI_ENABLE      = (1 << 7),
    PPUCTRL_LF_SELECT       = (1 << 6),
    PPUCTRL_SPRITE_SIZE     = (1 << 5),
    PPUCTRL_BG_PTABLE       = (1 << 4),
    PPUCTRL_SPRITE_PTABLE   = (1 << 3),
    PPUCTRL_VRAM_INC        = (1 << 2),
} ControlFlag;

typedef enum {
    PPUMASK_ENABLE_SPRITES  = (1 << 4),
    PPUMASK_ENABLE_BG       = (1 << 3),
} MaskFlag;

typedef enum {
    PPUSTATUS_VBLANK             = (1 << 7),
    PPUSTATUS_SPRITE_0_HIT       = (1 << 6),
    PPUSTATUS_SPRITE_OVERFLOW    = (1 << 5)
} StatusFlag;

void ppu2C02_init(ppu2C02 *ppu, ppu2C02_read_fn read, ppu2C02_write_fn write, void *ctx) {
    memset(ppu, 0, sizeof(*ppu));

    ppu->read = read;
    ppu->write = write;
    ppu->ctx = ctx;
    return;
}

void ppu2C02_reset(ppu2C02 *ppu __attribute((unused))) {
    return;
}

void ppu2C02_step(ppu2C02 *ppu) {
    bool visible_scanline = ppu->scanline >= 0 && ppu->scanline <= 239;
    bool visible_dot = ppu->dot >= 1 && ppu->dot <= 256;
    bool prerender_scanline = ppu->scanline == PRERENDER_SCANLINE;
    bool fetch_scanline = visible_scanline || prerender_scanline;

    // Pre-render phase
    if (prerender_scanline) {
        if (ppu->dot == 1) {
            // clear status flags
            ppu->status &= ~(PPUSTATUS_VBLANK | PPUSTATUS_SPRITE_OVERFLOW | PPUSTATUS_SPRITE_0_HIT);
            ppu->nmi_pending = 0;
        }
    }

    if (visible_scanline && visible_dot) {
        // render pixel from shift register value
    }

    if (rendering_enabled(ppu) && fetch_scanline) {
        if (visible_dot || (ppu->dot >= 321 && ppu->dot <= 336)) {
            update_bg_fetch_pipeline(ppu);
            // update shift registers
        }

        if (ppu->dot == 256) {
            // update vertical (v)
        }

        if (ppu->dot == 257) {
            // copy horizontal scroll (v -> t)
        }

        if (prerender_scanline && ppu->dot >= 280 && ppu->dot <= 304) {
            // copy vertical scroll (v -> t)
        }
    }

    // Vblank phase
    if (ppu->scanline == VBLANK_SCANLINE && ppu->dot == 1) {
        ppu->status |= PPUSTATUS_VBLANK;
        if (ppu->ctrl & PPUCTRL_NMI_ENABLE) {
            ppu->nmi_pending = 1;
        }
    }

    increment_scanline_position(ppu);
}

uint8_t ppu2C02_cpu_read(ppu2C02 *ppu, uint8_t reg) {
    switch (reg) {
        case 2: {
            // PPUSTATUS
            uint8_t result = ppu->status;
            ppu->status &= ~PPUSTATUS_VBLANK;
            ppu->w = 0;
            return result;
        }
        case 4: {
            // OAMDATA
            return ppu->oam[ppu->oam_addr];
        }
        case 7: {
            uint8_t result;

            if (ppu->v >= 0x3F00) {
                // palette reads are immediate
                result = ppu->read(ppu->ctx, ppu->v);
                ppu->data_buffer = ppu->read(ppu->ctx, ppu->v - 0x1000);
            } else {
                result = ppu->data_buffer;
                ppu->data_buffer = ppu->read(ppu->ctx, ppu->v);
            }

            ppu->v += (ppu->ctrl & PPUCTRL_VRAM_INC) ? 32 : 1;
            return result;
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
        case 1: {
            // PPUMASK
            ppu->mask = value;
            break;
        }
        case 3: {
            // OAMADDR
            ppu->oam_addr = value;
            break;
        }
        case 4: {
            // OAMDATA
            ppu->oam[ppu->oam_addr] = value;
            ppu->oam_addr++;
            break;
        }
        case 5: {
            // PPUSCROLL
            if (!ppu->w) {
                // first write
                ppu->t = (ppu->t & 0xFFE0) | (value >> 3); // coarse x
                ppu->x = value & 0x07; // fine x
                ppu->w = 1;
            } else {
                // second write
                ppu->t = (ppu->t & 0x8FFF) | ((value & 0x07) << 12); // fine y
                ppu->t = (ppu->t & 0xFC1F) | ((value & 0xF8) << 12); // coarse y
                ppu->w = 0;
            }
            break;
        }
        case 6: {
            // PPUADDR
            if (!ppu->w) {
                // first write
                ppu->t = (ppu->t & 0x00FF) | ((value & 0x3F) << 8);
                ppu->w = 1;
            } else {
                // second write
                ppu->t = (ppu->t & 0xFF00) | value;
                ppu->v = ppu->t;
                ppu->w = 0;
            }
            break;
        }
        case 7: {
            // PPUDATA
            ppu->write(ppu->ctx, ppu->v, value);
            ppu->v = (ppu->v + ((ppu->ctrl & PPUCTRL_VRAM_INC) ? 32 : 1)) & 0x3FFF;
            break;
        }
        default:
            break;
    }
}

/**
 * Progresses background tile fetching pipeline based
 * on current PPU cycle/scanline position.
 * 
 * The process takes 8 cycles to complete, with 2 cycles
 * per each phase:
 *  1. Fetch the nametable byte (pattern table tile) from VRAM
 *     based on current scanline position
 *  2. Fetch attribute byte (selected palette) from VRAM
 *     based on scanline position
 *  3. Fetch the "low" byte (Bitplane of LSBs of palette color indices for tile row)
 *     of the selected pattern table tile.
 *  4. Fetch the "high" byte (Bitplane of MSBs of palette color indices for tile row)
 *     of the selected pattern table tile
 * 
 *  Low and high bits will be combined to get the color palette
 *  index of the current pixel during rendering.
 *      
 *      L: 0 1 1 0 0 1 1 1
 *      H: 1 0 1 1 0 1 0 1
 *      R: 2 1 3 2 0 3 1 3
 * 
 *  This pipeline fetches the data needed for the
 *  next 8 background pixels and loads it into
 *  temporary latches / shift registers.
 */
void update_bg_fetch_pipeline(ppu2C02 *ppu) {
    switch ((ppu->dot - 1) % 8) {
        case 0: {
            load_shift_registers(ppu);

            uint16_t nt_addr = get_current_nametable_address(ppu);
            ppu->bg_tile = ppu->read(ppu->ctx, nt_addr);
            break;
        }
        case 2: {
            ppu->bg_palette = get_bg_palette_number(ppu);
            break;
        }
        case 4: {
            ppu->bg_pattern_addr = get_bg_pattern_address(ppu, ppu->bg_tile);
            ppu->bg_pattern_low = ppu->read(ppu->ctx, ppu->bg_pattern_addr + fine_y(ppu));
            break;
        }
        case 6: {
            ppu->bg_pattern_high = ppu->read(ppu->ctx, ppu->bg_pattern_addr + fine_y(ppu) + 8);
            break;
        }
        default: {
            break;
        }
    }
}

void load_shift_registers(ppu2C02 *ppu) {
    ppu->bg_pattern_low_shift =
        (ppu->bg_pattern_low_shift & 0xFF00) | ppu->bg_pattern_low;
    ppu->bg_pattern_high_shift =
        (ppu->bg_pattern_high_shift & 0xFF00) | ppu->bg_pattern_high;

    ppu->bg_palette_low_shift =
        (ppu->bg_palette_low_shift & 0xFF00)
        | ((ppu->bg_palette & 0x01 == 1) ? 0xFF : 0x00);
    ppu->bg_palette_high_shift =
        (ppu->bg_palette_high_shift & 0xFF00)
        | ((ppu->bg_palette & 0x2 == 1) ? 0xFF : 0x00);
}

/**
 * Update current scanline, dot, and frame
 */
void increment_scanline_position(ppu2C02 *ppu) {
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

/**
 * Determine if rendering is enabled by CPU based on PPUMASK flags
 */
bool rendering_enabled(ppu2C02 *ppu) {
    return (ppu->mask & (PPUMASK_ENABLE_SPRITES | PPUMASK_ENABLE_BG)) != 0;
}

/**
 * Fetch the tile index from the current nametable position.
 *
 * The address is derived from the current VRAM address (v),
 * which encodes the nametable selection, coarse X, and
 * coarse Y scroll position.
 */
uint8_t get_nametable_byte(ppu2C02 *ppu) {
    uint16_t nt_address = (ppu->v & 0x0FFF) + VRAM_START_ADDR;
    return ppu->read(ppu->ctx, nt_address);
}

/**
 * Fetch the address of the current nametable position.
 *
 * The address is derived from the current VRAM address (v),
 * which encodes the nametable selection, coarse X, and
 * coarse Y scroll position.
 */
uint16_t get_current_nametable_address(ppu2C02 *ppu) {
    return VRAM_START_ADDR | (ppu->v & 0x0FFF);
}

/**
 * Get the palette index of the current background tile
 */
uint8_t get_bg_palette_number(ppu2C02 *ppu) {
    uint16_t at_addr = get_current_attribute_address(ppu);
    uint8_t attribute_byte = ppu->read(ppu->ctx, at_addr);

    uint16_t cx = coarse_x(ppu);
    uint16_t cy = coarse_y(ppu);

    // Select which 2-bit quadrant within the attribute byte applies.
    // Each quadrant covers a 2x2 tile area.
    uint8_t shift =
        ((cy & 0x02) << 1)
        | (cx & 0x02);

    return (attribute_byte >> shift) & 0x03;
}

/**
 * Returns the address of the attribute byte corresponding
 * to the current VRAM position (v).
 *
 * The attribute table is an 8x8 grid located at the end of
 * each nametable. Each attribute byte provides palette
 * information for a 4x4 tile region, so the current coarse
 * X and coarse Y tile coordinates are divided by 4 to find
 * the attribute table entry covering the current tile.
 */
uint16_t get_current_attribute_address(ppu2C02 *ppu) {
    uint16_t nametable_start_addr = VRAM_START_ADDR | (ppu->v & 0x0C00);
    uint16_t attribute_table_start = nametable_start_addr + NT_SIZE;

    uint16_t attr_x = coarse_x(ppu) / 4;
    uint16_t attr_y = coarse_y(ppu) / 4;

    uint16_t attribute_address = attribute_table_start + (attr_y * 8) + attr_x;
    return attribute_address;
}

/**
 * Returns the address of the pattern table tile of the
 * current nametable tile.
 */
uint16_t get_bg_pattern_address(ppu2C02 *ppu, uint16_t tile_num) {
    uint16_t pattern_table_base = 0;
    uint16_t pattern_table_num = (ppu->ctrl >> 4) & 0x01;

    return pattern_table_base + (pattern_table_num * 0x1000) + (tile_num << 4);
}

/**
 * Returns the coarse X scroll component from the
 * current VRAM address (v).
 *
 * Coarse X is the horizontal tile position within
 * the current nametable (0-31).
 */
uint16_t coarse_x(ppu2C02 *ppu) {
    return ppu->v & 0x001F;
}

/**
 * Returns the coarse Y scroll component from the
 * current VRAM address (v).
 * 
 * Coarse Y is the vertical tile position within
 * the current nametable (0-29)
 */
uint16_t coarse_y(ppu2C02 *ppu) {
    return (ppu->v >> 5) & 0x001F;
}

/**
 * Returns the fine Y scroll component from the
 * current VRAM address (v)
 * 
 * Fine Y is the vertical dot position within
 * the current nametable
 */
uint16_t fine_y(ppu2C02 *ppu) {
    return (ppu->v >> 12) & 0x07;
}