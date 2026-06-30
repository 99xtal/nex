#ifndef NEX_NES_H
#define NEX_NES_H

#include <lib6502/6502.h>

typedef struct nes nes;

nes* nex_create(void);

int nex_load_rom(nes* n, const char* path);

void nex_reset(nes* n);

void nex_step(nes* n);

void nex_free(nes* n);

#endif