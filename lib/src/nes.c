#include "nes.h"

#include <nex/nes.h>
#include <stdio.h>
#include <string.h>

int nex_init(NES* n);

uint8_t nes_cpu_read(void* ctx, uint16_t addr) {
  NES* n = ctx;

  if (addr < 0x2000) {
    return n->wram[addr % 0x0800];
  }

  if (addr >= 0x2000 && addr < 0x4000) {
    // PPU MMIO registers
    uint8_t register_value = (addr - 0x2000) % 8;
    return ppu_cpu_read(&n->ppu, register_value);
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

void nes_cpu_write(void* ctx, uint16_t addr, uint8_t value) {
  NES* n = ctx;

  if (addr < 0x2000) {
    n->wram[addr % 0x0800] = value;
  }

  if (addr >= 0x2000 && addr < 0x4000) {
    // PPU MMIO registers
    uint8_t register_value = (addr - 0x2000) % 8;
    ppu_cpu_write(&n->ppu, register_value, value);
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

uint8_t nes_ppu_read(void* ctx, uint16_t addr) {
  NES* n = ctx;

  // CHROM addressing
  if (addr < 0x2000) {
    return n->cartridge->mapper.ppu_read(&n->cartridge->mapper, addr);
  }

  // VRAM addressing
  // mirror 0x2000-0x2FFF address space
  if (addr >= 0x3000 && addr < 0x3F00) {
    addr -= 0x1000;
  }

  if (addr >= 0x2000 && addr < 0x3000) {
    uint16_t offset =
        (addr - 0x2000) % 0x1000;     // address within nametable space
    uint16_t table = offset / 0x400;  // which nametable you're in (1-4)
    uint16_t inner = offset % 0x400;  // offset from current nametable (1-4)

    NametableMirroring mirroring_mode = n->cartridge->nt_mirroring;
    if (mirroring_mode == NT_MIRROR_HORIZONTAL) {
      uint16_t vram_addr = (table / 2) * 0x400 + inner;
      return n->vram[vram_addr];
    } else if (mirroring_mode == NT_MIRROR_VERTICAL) {
      uint16_t vram_addr = (table % 2) * 0x400 + inner;
      return n->vram[vram_addr];
    }
  }

  if (addr >= 0x3F00 && addr < 0x3FFF) {
    uint16_t palette_ram_addr = (addr - 0x3F00) % 0x20;
    return n->ppu.palette_ram[palette_ram_addr];
  }

  return 0;
}

void nes_ppu_write(void* ctx, uint16_t addr, u_int8_t value) {
  NES* n = ctx;

  // CHROM addressing
  if (addr < 0x2000) {
    n->cartridge->mapper.ppu_write(&n->cartridge->mapper, addr, value);
  }

  // VRAM addressing
  // mirror 0x2000-0x2FFF address space
  if (addr >= 0x3000 && addr < 0x3F00) {
    addr -= 0x1000;
  }

  if (addr >= 0x2000 && addr < 0x3000) {
    uint16_t offset =
        (addr - 0x2000) % 0x1000;     // address within nametable space
    uint16_t table = offset / 0x400;  // which nametable you're in (1-4)
    uint16_t inner = offset % 0x400;  // offset from current nametable (1-4)

    NametableMirroring mirroring_mode = n->cartridge->nt_mirroring;
    if (mirroring_mode == NT_MIRROR_HORIZONTAL) {
      uint16_t vram_addr = (table / 2) * 0x400 + inner;
      n->vram[vram_addr] = value;
      return;
    } else if (mirroring_mode == NT_MIRROR_VERTICAL) {
      uint16_t vram_addr = (table % 2) * 0x400 + inner;
      n->vram[vram_addr] = value;
      return;
    }
  }

  if (addr >= 0x3F00 && addr < 0x3FFF) {
    uint16_t palette_ram_addr = (addr - 0x3F00) % 0x20;
    n->ppu.palette_ram[palette_ram_addr] = value;
  }
}

NES* nex_create(void) {
  NES* n = malloc(sizeof(*n));
  if (!n) return NULL;

  if (nex_init(n) != 0) {
    free(n);
    return NULL;
  }

  return n;
}

int nex_init(NES* n) {
  if (!n) {
    return -1;
  }

  memset(n, 0, sizeof(*n));
  n->total_cpu_cycles = 0;

  n->cartridge = malloc(sizeof(*n->cartridge));
  if (!n->cartridge) {
    return -1;
  }

  n->cpu =
      cpu6502_create(CPU6502_VARIANT_RP2A03, nes_cpu_read, nes_cpu_write, n);

  ppu_init(&n->ppu, nes_ppu_read, nes_ppu_write, n);

  return 0;
}

int nex_load_rom(NES* n, const char* path) {
  return cartridge_load(n->cartridge, path);
}

void nex_reset(NES* n) {
  if (!n || !n->cartridge->prg_rom || !n->cpu) {
    return;
  }

  ppu_reset(&n->ppu);

  cpu6502_reset(n->cpu);
  for (int i = 0; i < 3; i++) {
    ppu_step(&n->ppu);
  }

  n->total_cpu_cycles++;
}

void nex_tick(NES* n) {
  if (n->ppu.nmi_pending) {
    n->ppu.nmi_pending = 0;
    cpu6502_nmi(n->cpu);
  } else {
    cpu6502_tick(n->cpu);
  }

  for (int i = 0; i < 3; i++) {
    ppu_step(&n->ppu);
  }

  if (n->ppu.frame_ready) {
    n->ppu.frame_ready = false;

    // convert PPU color indices into a framebuffer
    // execute framebuffer callback
  }

  n->total_cpu_cycles++;
}

int nex_step(NES* n) {
  int cpu_cycles;

  if (n->ppu.nmi_pending) {
    n->ppu.nmi_pending = 0;
    // Todo: fix this
    cpu6502_nmi(n->cpu);
  } else {
    cpu_cycles = cpu6502_step(n->cpu);
  }

  // PPU ticks 3 times for every CPU cycle
  for (int i = 0; i < cpu_cycles * 3; i++) {
    ppu_step(&n->ppu);
  }

  if (n->ppu.frame_ready) {
    n->ppu.frame_ready = false;

    // convert PPU color indices into a framebuffer
    // execute framebuffer callback
  }

  n->total_cpu_cycles += cpu_cycles;

  return cpu_cycles;
}

NexCpuState nex_get_cpu_state(NES* n) {
  CPU6502State cpu_state = cpu6502_get_state(n->cpu);

  return (NexCpuState){
      .PC = cpu_state.PC,
      .A = cpu_state.A,
      .X = cpu_state.X,
      .Y = cpu_state.Y,
      .P = cpu_state.status,
      .SP = cpu_state.SP,
      .scanline = n->ppu.scanline,
      .dot = n->ppu.dot,
      .total_cycles = n->total_cpu_cycles,
  };
}

NexPpuState nex_get_ppu_state(NES* n) {
  return (NexPpuState){
      .frame = n->ppu.frame,
      .scanline = n->ppu.scanline,
      .dot = n->ppu.dot,
      .mask = n->ppu.mask,
      .status = n->ppu.status,
      .ctrl = n->ppu.ctrl,
  };
}

void nex_read_wram(const NES* n, uint8_t dst[NEX_WRAM_SIZE]) {
  if (!n || !dst) {
    return;
  }

  memcpy(dst, n->wram, NEX_WRAM_SIZE);
}

void nex_read_vram(const NES* n, uint8_t dst[NEX_VRAM_SIZE]) {
  if (!n || !dst) {
    return;
  }

  memcpy(dst, n->vram, NEX_VRAM_SIZE);
}

void nex_free(NES* n) {
  if (!n) {
    return;
  }

  free(n->cartridge);
  free(n);
}

void nex_step_scanline(NES* n) {
  int16_t current_scanline = n->ppu.scanline;

  while (n->ppu.scanline == current_scanline) {
    nex_tick(n);
  }
}

void nex_step_frame(NES* n) {
  uint64_t current_frame = n->ppu.frame;

  while (n->ppu.frame == current_frame) {
    nex_tick(n);
  }
}