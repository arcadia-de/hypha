#include "hypha/action_log.h"

#include <string.h>

#include "hypha/assertions.h"
#include "hypha/log.h"
#include "hypha/structured_log.h"

void SortActionLog(AppliedActionLog* alog) {
  ASSERT(alog);
  qsort(alog->data, alog->data_len, sizeof(AppliedAction), &CompareAppliedAction);
}

static inline void FreeAppliedAction(AppliedAction* rhs) {
  ASSERT(rhs);
  // do something?
}

DEFINE_STRUCTURED_LOG(AppliedAction);
