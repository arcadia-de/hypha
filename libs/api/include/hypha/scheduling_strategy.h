#ifndef HYPHA_SCHEDULING_STRATEGY_H
#define HYPHA_SCHEDULING_STRATEGY_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define FOR_EACH_SCHEDULING_STRATEGY(V) \
  V(PriorityWeightedKahn)               \
  V(DepthFirst)

// clang-format off
typedef enum {
#define DEFINE_KIND(Name) k##Name##Scheduling,
  FOR_EACH_SCHEDULING_STRATEGY(DEFINE_KIND)
#undef DEFINE_KIND
  kTotalNumberOfSchedulingStrategies,
} SchedulingStrategy;
// clang-format on

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_SCHEDULING_STRATEGY_H
