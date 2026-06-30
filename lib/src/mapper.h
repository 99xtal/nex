#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>

typedef struct mapper mapper;
typedef struct cartridge cartridge;

struct mapper {
  void* ctx;

  uint8_t (*cpu_read)(mapper* m, uint16_t addr);
  void (*cpu_write)(mapper* m, uint16_t addr, uint8_t value);

  uint8_t (*ppu_read)(mapper* m, uint16_t addr);
  void (*ppu_write)(mapper* m, uint16_t addr, uint8_t value);
};

// mapper 0
void mapper_nrom_init(mapper* m, cartridge* c);

#endif  // MAPPER_H