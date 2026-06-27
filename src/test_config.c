#include "test_config.h"

void test_config_init(TestConfig *cfg) {
  *cfg = (TestConfig){
    .start_pc = 0,
    .stop_pc = 0,
    .assertions = NULL,
    .assertion_cnt = 0,
    .assertion_cap = 0,
    .timeout_cycles = 1000000,
  };
}

void test_config_free(TestConfig *cfg) {
  free(cfg->assertions);
  *cfg = (TestConfig){0};
}

int test_config_add_assertion(TestConfig *cfg, Assertion assertion) {
  if (cfg->assertion_cnt == cfg->assertion_cap) {
    size_t new_cap = cfg->assertion_cap == 0 ? 4 : cfg->assertion_cap * 2;

    Assertion *new_assertions =
      realloc(cfg->assertions, new_cap * sizeof(*new_assertions));

    if (!new_assertions) {
      return -1;
    }

    cfg->assertions = new_assertions;
    cfg->assertion_cap = new_cap;
  }

  cfg->assertions[cfg->assertion_cnt++] = assertion;
  return 0;
}