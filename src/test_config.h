#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include <stdint.h>
#include <stdlib.h>

typedef enum {
    ASSERT_MEM_EQ,
} AssertionType;

typedef struct {
  AssertionType type;

  union {
    struct {
      uint16_t addr;
      uint8_t value;
    } mem;
  };
} Assertion;

typedef struct TestConfig {
  uint16_t start_pc;
  uint16_t stop_pc;

  Assertion *assertions;
  size_t assertion_cnt;
  size_t assertion_cap;

  uint64_t timeout_cycles;
} TestConfig;

void test_config_init(TestConfig *cfg);
void test_config_free(TestConfig *cfg);
int test_config_add_assertion(TestConfig *cfg, Assertion assertion);

#endif