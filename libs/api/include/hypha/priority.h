#ifndef HYPHA_PRIORITY_H
#define HYPHA_PRIORITY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef uint32_t Priority;

static const Priority kMinPriority = 1;
static const Priority kMaxPriority = 100;
static const Priority kDefaultPriority = 10;

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_PRIORITY_H
