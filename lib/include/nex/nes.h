#ifndef NEX_NES_H
#define NEX_NES_H

#include <lib6502/6502.h>

typedef struct NES NES;

NES* nex_create(void);

int nex_load_rom(NES* n, const char* path);

void nex_reset(NES* n);

void nex_step(NES* n);

void nex_free(NES* n);

#endif