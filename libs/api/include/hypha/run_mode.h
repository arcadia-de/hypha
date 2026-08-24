#ifndef HYPHA_RUN_MODE_H
#define HYPHA_RUN_MODE_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define FOR_EACH_ORCHESTRATOR_RUN_MODE(V) \
  V(Plan)                                 \
  V(Diff)                                 \
  V(Validate)                             \
  V(Apply)                                \
  V(Destroy)

// clang-format off
typedef enum {
#define DEFINE_MODE(Name) kOrchestrator##Name##Mode,
  FOR_EACH_ORCHESTRATOR_RUN_MODE(DEFINE_MODE)
#undef DEFINE_MODE
  kTotalNumberOfOrchestratorRunModes,
  kDefaultOrchestratorMode = kOrchestratorApplyMode,
} OrchestratorRunMode;
// clang-format on

static inline const char* OrchestratorRunModeName(const OrchestratorRunMode rhs) {
  switch (rhs) {
#define DEFINE_TOSTRING(Name)     \
  case kOrchestrator##Name##Mode: \
    return #Name;
    FOR_EACH_ORCHESTRATOR_RUN_MODE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
    default:
      return "Unknown";
  }
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_RUN_MODE_H
