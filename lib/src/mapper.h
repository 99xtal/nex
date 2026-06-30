#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>

typedef struct Mapper Mapper;
typedef struct Cartridge Cartridge;

struct Mapper {
  void* ctx;

  uint8_t (*cpu_read)(Mapper* m, uint16_t addr);
  void (*cpu_write)(Mapper* m, uint16_t addr, uint8_t value);

  uint8_t (*ppu_read)(Mapper* m, uint16_t addr);
  void (*ppu_write)(Mapper* m, uint16_t addr, uint8_t value);
};

// mapper 0
void mapper_nrom_init(Mapper* m, Cartridge* c);

#endif  // MAPPER_H