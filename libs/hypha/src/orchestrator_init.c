#include <ctype.h>
#include <dirent.h>
#include <fnmatch.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "bootstrap.h"
#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/history.h"
#include "hypha/log.h"
#include "hypha/orchestrator.h"

#ifdef HYPHA_ENABLE_PROFILING

static inline void OnProfilingCheck(uv_check_t* handle) {
  TracyCFrameMark;
}

#endif  // HYPHA_ENABLE_PROFILING

static inline void InitOrcState(Orchestrator* orc) {
  ASSERT(orc->config.state_dir);
  char p[PATH_MAX];
  snprintf(p, PATH_MAX, "%s/journal.log", orc->config.state_dir);
  orc->state = StateStoreOpen(p);
  LOG_FATAL_IF(!orc->state, "failed to open orchestrator state: %s", p);
}

static inline void InitOrcHistory(Orchestrator* orc) {
  ASSERT(orc->config.state_dir);
  char p[PATH_MAX];
  snprintf(p, PATH_MAX, "%s/history.log", orc->config.state_dir);
  orc->history = HistoryLogOpen(p, 5 * 1024 * 1024, 2);
  LOG_FATAL_IF(!orc->history, "failed to open history log: %s", p);
}

static inline bool StopLoopOnReconcileDone(const char* p, const void* event, void* data) {
  ASSERT(event);
  Orchestrator* orc = (Orchestrator*)data;
  uv_stop(orc->loop);
  return true;
}

static inline bool OnReconcileComplete(const char* p, const void* event, void* data) {
  return true;
}

static inline bool FailedToExecuteInit(FILE* out, Orchestrator* orc, const char* path) {
  ASSERT(out);
  ASSERT(orc);
  ASSERT(path);
#define L orc->L
  const char* err = lua_tostring(L, -1);
  LOG_ERROR("failed to execute init file %s: %s", path, err);
#undef L
  return false;
}

static inline bool ExecInit(Orchestrator* orc) {
  char path[PATH_MAX];
  snprintf(path, PATH_MAX, "%s/init.lua", orc->config.root);
  const int result = luaL_dofile(orc->L, path);  // TODO(@s0cks): check result
  if (result != LUA_OK)
    return FailedToExecuteInit(stderr, orc, path);
  return true;
}

static inline bool OnGraphSubmitted(const char* p, const void* event, void* data) {
  Orchestrator* orc = (Orchestrator*)data;
  if (!ComputeExecutionSchedule(orc->graph, kPriorityWeightedKahnScheduling)) {
    orc->run.status = kStatusInternalError;
    OrchestratorPublish(orc, RECONCILE_FAILED_EVENT, NewReconcileFailedEvent(kStatusInvalidSpec));
    goto finished;
  }

  if (IsResourceGraphEmpty(orc->graph)) {
    OrchestratorPublish(orc, RECONCILE_COMPLETE_EVENT, NewReconcileCompleteEvent(kStatusOk));
    goto finished;
  }

  const size_t num_resources = GetNumberOfResourcesInResourceGraph(orc->graph) + 1;
  InitPlan(&orc->plan, num_resources);
  InitAppliedActionLog(&orc->actions, num_resources);
  InitDeltaLog(&orc->dlog, num_resources);
  InitValidationLog(&orc->vlog, num_resources);

  DispatchReadyResources(orc);
finished:
  return true;
}

typedef bool (*WalkContextCallbackFn)(uint32_t idx, const char* path, void* data);

typedef struct {
  WalkContextCallbackFn fn;
  void* data;

  char** patterns;
  size_t patterns_len;

  size_t num_files;
} WalkContext;

typedef struct {
  lua_State* L;
  int result_index;
} LuaWalkData;

static inline void WalkDir(const char* base_path, WalkContext* ctx) {
  DIR* dir = opendir(base_path);
  if (!dir)
    return;

  char path[PATH_MAX];
  struct dirent* dp = NULL;
  while ((dp = readdir(dir)) != NULL) {
    if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
      snprintf(path, sizeof(path), "%s/%s", base_path, dp->d_name);

      struct stat statbuf;
      if (stat(path, &statbuf) == 0) {
        if (S_ISDIR(statbuf.st_mode)) {
          WalkDir(path, ctx);
        } else if (S_ISREG(statbuf.st_mode)) {
          for (size_t i = 0; i < ctx->patterns_len; i++) {
            const char* pattern = ctx->patterns[i];
            if (fnmatch(pattern, dp->d_name, 0) == 0) {
              if (ctx->fn) {
                if (!ctx->fn(ctx->num_files, path, ctx->data))
                  goto finished;
                ctx->num_files++;
              }
            }
          }
        }
      }
    }
  }
finished:
  closedir(dir);
}

static inline bool OnDefaultSourceFound(uint32_t idx, const char* p, void* data) {
#define L ((LuaWalkData*)data)->L
  lua_newtable(L);
  const int new_tbl_idx = lua_gettop(L);
  lua_pushnumber(L, kDiscoveredPath);
  lua_setfield(L, new_tbl_idx, "kind");

  lua_pushstring(L, p);
  lua_setfield(L, new_tbl_idx, "value");

  lua_rawseti(L, -2, idx + 1);
#undef L
  return true;
}

static inline int getDefaultSources(lua_State* L) {
  WalkContext ctx;
  ctx.patterns = (char**)malloc(sizeof(char*) * 3);
  ctx.patterns[0] = "*.jsonnet";
  ctx.patterns[1] = "*.json";
  ctx.patterns[2] = "*.yaml";
  ctx.patterns_len = 3;
  ctx.fn = &OnDefaultSourceFound;
  ctx.num_files = 0;

  lua_newtable(L);
  int result_index = lua_gettop(L);
  LuaWalkData data = {
      .L = L,
      .result_index = result_index,
  };
  ctx.data = &data;
  WalkDir("/home/tazz/.config/hypha", &ctx);
  return 1;
}

OrchestratorHandle NewOrchestrator(OrchestratorConfig config) {
  if (!config.root)
    return NULL;

  Orchestrator* orc = (Orchestrator*)malloc(sizeof(Orchestrator));
  if (orc) {
    memset(orc, 0, sizeof(Orchestrator));
    orc->config.root = strdup(config.root);
    orc->config.state_dir = strdup(config.state_dir);
    orc->config.cache_dir = strdup(config.cache_dir);
    orc->loop = uv_default_loop();
    orc->graph = NewResourceGraph();
    BootstrapHyphaCoreResources(orc->graph);
    orc->discovered_manifests = NULL;
    orc->num_discovered_manifests = 0;
#ifdef HYPHA_ENABLE_PROFILING
    uv_check_init(orc->loop, &orc->profiling_check);
    orc->profiling_check.data = orc;
    uv_check_start(&orc->profiling_check, &OnProfilingCheck);
#endif  // HYPHA_ENABLE_PROFILING

    InitOrcState(orc);
    InitOrcHistory(orc);
    orc->bus = (EventBus*)malloc(sizeof(EventBus));
    InitEventBus(orc->loop, orc->bus);
    orc->L = NewOrchestratorLuaState(orc);
    LOG_FATAL_IF(!orc->L, "failed to create orchestrator lua state");

    OrchestratorSubscribe(orc, GRAPH_SUBMITTED_EVENT, &OnGraphSubmitted, orc, NULL);
    OrchestratorSubscribe(orc, RECONCILE_COMPLETE_EVENT, &OnReconcileComplete, orc, NULL);
    OrchestratorSubscribe(orc, RECONCILE_COMPLETE_EVENT, &StopLoopOnReconcileDone, orc, NULL);
    OrchestratorSubscribe(orc, RECONCILE_FAILED_EVENT, &StopLoopOnReconcileDone, orc, NULL);

    if (!ExecInit(orc)) {
      getDefaultSources(orc->L);
    } else {
      const int nresults = lua_gettop(orc->L);
      if (nresults == 1) {
        if (lua_isnil(orc->L, -1)) {
          getDefaultSources(orc->L);
        }
      } else if (nresults == 0) {
        getDefaultSources(orc->L);
      }
    }

    DiscoverManifestPaths(orc->L, &orc->discovered_manifests, &orc->num_discovered_manifests);
  }

  OrchestratorPublish(orc, ORCHESTRATOR_INIT_EVENT, NewOrchestratorInitEvent());
  return (OrchestratorHandle)orc;
}
