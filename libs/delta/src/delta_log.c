#include "hypha/delta_log.h"

#include <stdint.h>
#include <string.h>

#include "hypha/assertions.h"
#include "hypha/log.h"
#include "hypha/structured_log.h"

static inline void FreeDelta(Delta* delta) {
  ASSERT(delta);
  // do something?
}

DEFINE_STRUCTURED_LOG(Delta);
