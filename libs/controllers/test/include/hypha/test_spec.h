#ifndef HYPHA_TEST_SPEC_H
#define HYPHA_TEST_SPEC_H

#include <stdint.h>

static const uint32_t kDefaultSleepSeconds = 1;

typedef struct {
  uint32_t sleep;
} TestSpec;

#endif  // HYPHA_TEST_SPEC_H
