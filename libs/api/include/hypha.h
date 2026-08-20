#ifndef HYPHA_H
#define HYPHA_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdlib.h>

typedef enum {
  kDiscoveredPath,
  kDiscoveredRaw,
} DiscoveredManifestKind;

#ifdef HYPHA_DEBUG

#include <assert.h>

#ifndef ASSERT
#define ASSERT(x) assert((x))
#endif  // ASSERT

#ifndef ASSERT_EQ
#define ASSERT_EQ(a, b) ASSERT(a == b)
#endif  // ASSERT_EQ

#ifndef ASSERT_NE
#define ASSERT_NE(a, b) ASSERT(a != b)
#endif  // ASSERT_NE

#else

#ifndef ASSERT
#define ASSERT(x)
#endif  // ASSERT

#ifndef ASSERT_EQ
#define ASSERT_EQ(a, b)
#endif  // ASSERT_EQ

#ifndef ASSERT_NE
#define ASSERT_NE(a, b)
#endif  // ASSERT_NE

#endif  // HYPHA_DEBUG

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

#define LUA_REGISTRY_ORC_KEY    "hypha_orchestrator"
#define LUA_REGISTRY_EVENTS_KEY "hypha_events"

#ifndef container_of
#define container_of(ptr, type, member)               \
  ({                                                  \
    const typeof(((type*)0)->member)* __mptr = (ptr); \
    (type*)((char*)__mptr - offsetof(type, member));  \
  })
#endif  // container_of

#ifndef HYPHA_REASON_MAX_LENGTH
#define HYPHA_REASON_MAX_LENGTH 128
#endif  // HYPHA_REASON_MAX_LENGTH

typedef char Reason[HYPHA_REASON_MAX_LENGTH];

typedef struct _Resource Resource;

typedef void* OrchestratorHandle;

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

#define FOR_EACH_CONTROLLER_STATUS(V) \
  V(Ok)                               \
  V(NoOp)                             \
  V(InvalidSpec)                      \
  V(NotFound)                         \
  V(Conflict)                         \
  V(Unsupported)                      \
  V(TransientError)                   \
  V(PermanentError)                   \
  V(InternalError)

// clang-format off
typedef enum {
#define DEFINE_STATUS(Name) kStatus##Name,
  FOR_EACH_CONTROLLER_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
  kTotalNumberOfControllerStatuses,
} ControllerStatus;
// clang-format on

static inline const char* ControllerStatusToCString(const ControllerStatus rhs) {
  switch (rhs) {
#define DEFINE_TOSTRING(Name) \
  case kStatus##Name:         \
    return #Name;

    FOR_EACH_CONTROLLER_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
    default:
      return "Unknown";
  }
}

#define FOR_EACH_CONTROLLER_ACTION(V) \
  V(No)                               \
  V(Create)                           \
  V(Update)                           \
  V(Destroy)

// clang-format off
typedef enum {
#define DEFINE_ACTION(Name) k##Name##Action,
  FOR_EACH_CONTROLLER_ACTION(DEFINE_ACTION)
#undef DEFINE_ACTION
  kTotalNumberOfControllerActions,
} ControllerAction;
// clang-format on

static inline const char* ControllerActionToCString(const ControllerAction rhs) {
  switch (rhs) {
#define DEFINE_TOSTRING(Name) \
  case k##Name##Action:       \
    return #Name;

    FOR_EACH_CONTROLLER_ACTION(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
    default:
      return "Unknown";
  }
}

#define FOR_EACH_ORCHESTRATOR_RUN_MODE(V) \
  V(Plan)                                 \
  V(Diff)                                 \
  V(Destroy)                              \
  V(Apply)

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

typedef uint64_t ControllerActionCounts[kTotalNumberOfControllerActions];

typedef struct {
  uint64_t run_start;
  uint64_t run_finished;
  uint64_t num_processed;
  ControllerActionCounts num_actions;
} OrchestratorMetrics;

typedef struct _HistoryLog HistoryLog;

void InitHypha(const char* luarocks_dir);
extern char* RenderJsonnet(char* name, char* code);
extern char* RenderTemplate(char* tpl, char* data, bool is_yaml);
extern uint64_t ValidateManifests(char** tpls, uint64_t num_tpls, bool* valid);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_H
