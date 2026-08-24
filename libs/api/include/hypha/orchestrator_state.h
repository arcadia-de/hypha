#ifndef HYPHA_ORCHESTRATOR_STATE_H
#define HYPHA_ORCHESTRATOR_STATE_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define FOR_EACH_ORCHESTRATOR_STATE(V) \
  V(Observe)                           \
  V(Normalize)                         \
  V(Validate)                          \
  V(Plan)                              \
  V(Apply)                             \
  V(Destroy)                           \
  V(Diff)                              \
  V(Status)                            \
  V(Rollback)

// clang-format off
typedef enum {
#define DEFINE_STATE(Name) k##Name##State,
  FOR_EACH_ORCHESTRATOR_STATE(DEFINE_STATE)
#undef DEFINE_STATE
  kTotalNumberOfOrchestratorStates,
} OrchestratorState;
// clang-format on

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_ORCHESTRATOR_STATE_H
