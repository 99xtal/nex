#ifndef NEX_NES_H
#define NEX_NES_H

#include <lib6502/6502.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NES NES;

NES* nex_create(void);

int nex_load_rom(NES* n, const char* path);

void nex_reset(NES* n);

void nex_step(NES* n);

void nex_free(NES* n);

typedef struct NexCpuState {
  uint16_t PC;
  uint8_t A;
  uint8_t X;
  uint8_t Y;
  uint8_t P;
  uint8_t SP;
  uint64_t total_cycles;
} NexCpuState;

NexCpuState nex_get_cpu_state(NES* n);

typedef struct NexDisasmLine {
  uint16_t addr;
  uint8_t bytes[3];
  size_t bytes_count;
  char* mnemonic;
  char operand[25];
} NexDisasmLine;

bool nex_disassemble_at(NES* n, uint16_t addr, NexDisasmLine* out);

#ifdef __cplusplus
}
#endif

#endif