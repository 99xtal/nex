#ifndef PPU_H
#define PPU_H

#include <stdbool.h>
#include <stdint.h>

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 240

#define SCANLINE_MAX 261
#define DOT_MAX 340

#define VBLANK_SCANLINE 241
#define PRERENDER_SCANLINE 261

typedef uint8_t (*PPUReadFn)(void* ctx, uint16_t address);
typedef void (*PPUWriteFn)(void* ctx, uint16_t address, uint8_t value);

typedef struct PPU {
  // PPU state
  int16_t scanline;  // 0-239 visible, 240 post-render, 241-260 vblank, 261
                     // pre-render
  uint16_t dot;      // 0-340, aka PPU cycle within scanline
  uint64_t frame;
  int nmi_pending;  // boolean for when NMI is queued by "enable Vblank NMI"
                    // control flag

  // BG data fetching pipeline state
  uint8_t bg_tile;     // Current nametable byte (pattern table index)
  uint8_t bg_palette;  // Current attribute table bits (palette index)
  uint16_t
      bg_pattern_addr;  // Address of current background tile from pattern table
  uint8_t bg_pattern_low;   // Bitplane 0 of palette color indices of pattern
                            // tile row
  uint8_t bg_pattern_high;  // Bitplane 1 of palette color indices of pattern
                            // tile row

  uint16_t bg_palette_low_shift;
  uint16_t bg_palette_high_shift;
  uint16_t bg_pattern_low_shift;
  uint16_t bg_pattern_high_shift;

  // MMIO registers
  /**
   * PPUCTRL (W)
   * bit 7: Vblank NMI enable (0: off, 1: on)
   * bit 6: PPU leader/follower select
   *          (0: read backdrop from EXT pins,
   *           1: output color on EXT pins)
   * bit 5: Sprite size (0: 8x8 pixels, 1: 8x16 pixels)
   * bit 4: Background pattern table addr (0: $0000, 1: $1000)
   * bit 3: Sprite pattern table address for 8x8 sprites
   *          (0: $0000, 1: $1000; ignored in 8x16 mode)
   * bit 2: VRAM addr increment per CPU read/write of PPUDATA
   *          (0: add 1, going accross;
   *           1: add 32, going down)
   * bits 1-0: Base nametable address
   *          (0: $2000, 1: $2400
   *           2: $2800, 3: $2C00)
   */
  uint8_t ctrl;
  /**
   * PPUMASK (W)
   * bit 7: Emphasize blue
   * bit 6: Emphasize green (red on PAL)
   * bit 5: Emphasize red (green on PAL)
   * bit 4: Enable sprite rendering
   * bit 3: Enable background rendering
   * bit 2: Show (1)/Hide (0) sprites in leftmost 8px of screen
   * bit 1: Show (1)/Hide (0) background in leftmost 8px of screen
   * bit 0: Greyscale (0: color, 1: greyscale)
   */
  uint8_t mask;
  /**
   * PPUSTATUS (R)
   * bit 7: Vblank flag, cleared on read
   * bit 6: Sprite 0 hit flag
   * bit 5: Sprite overflow flag
   */
  uint8_t status;

  // internal registers
  /**
   * Rendering: used for scroll position
   *
   * yyy NN YYYYY XXXXX
   * ||| || ||||| +++++-- coarse X scroll
   * ||| || +++++-------- coarse Y scroll
   * ||| ++-------------- nametable select
   * +++----------------- fine Y scroll
   *
   * Non-rendering: current VRAM addr
   */
  uint16_t v;
  /**
   * Rendering: starting coarse-x scroll for next scanline
   * and starting y scroll for screen
   * Non-rendering: scroll or VRAM addr before transfer to 'v'
   */
  uint16_t t;
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

  uint8_t oam[0x100];  // Object Attribute Memory
  uint8_t oam_addr;    // $2003 (OAMADDR MMIO register)

  uint8_t palette_ram[0x20];

  uint8_t data_buffer;  // for PPUDATA reads

  uint8_t open_bus;

  // callbacks for PPU address-space reads/writes: $0000-$3FFF
  PPUReadFn read;
  PPUWriteFn write;
  void* ctx;

  uint8_t color_indices[SCREEN_WIDTH * SCREEN_HEIGHT];
  bool frame_ready;
} PPU;

void ppu_init(PPU* ppu, PPUReadFn read, PPUWriteFn write, void* ctx);

void ppu_reset(PPU* ppu);

void ppu_step(PPU* ppu);

// functions to be used by CPU to read/write to MMIO registers
uint8_t ppu_cpu_read(PPU* ppu, uint8_t reg);
void ppu_cpu_write(PPU* ppu, uint8_t reg, uint8_t value);

#endif  // PPU_H