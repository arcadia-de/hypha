#include "hypha/service_manager.h"

#include <stdlib.h>
#include <string.h>

#include "hypha/assertions.h"

struct _ServiceManager {
  uint64_t id;
  char* name;
  ServiceManagerConfig config;
  void* data;
  void (*free_data)(void*);
};

static const size_t kInitCap = 10;
static ServiceManager* managers = NULL;
static size_t managers_len = 0;
static size_t managers_cap = 0;

static inline bool EnsureCapacity(const size_t new_len) {
  if (new_len < managers_cap)
    return true;

  const size_t new_cap = managers_cap + new_len;
  const size_t total_size = sizeof(ServiceManager) * new_cap;
  ServiceManager* new_managers = (ServiceManager*)realloc(managers, total_size);
  if (!new_managers)
    return false;
  managers = new_managers;
  managers_cap = new_cap;
  return true;
}

static inline void InitServiceManager(ServiceManager* sm) {
  ServiceManagerConfig* config = &sm->config;
  if (!sm || !config->init)
    return;

  config->init(sm->data);
}

static inline void DeInitServiceManager(ServiceManager* sm) {
  ServiceManagerConfig* config = &sm->config;
  if (!sm || !config->init)
    return;

  config->deinit(sm->data);
}

ServiceManager* NewServiceManager(const char* name, const ServiceManagerConfig* config, void* data,
                                  void (*free_data)(void*)) {
  if (!name || !config)
    return NULL;

  if (managers == NULL) {
    const size_t total_size = sizeof(ServiceManager) * kInitCap;
    ServiceManager* new_managers = (ServiceManager*)malloc(total_size);
    if (!new_managers)
      return NULL;
    memset(new_managers, 0, total_size);
    managers = new_managers;
    managers_len = 0;
    managers_cap = kInitCap;
  }

  if (!EnsureCapacity(managers_len + 1))
    return NULL;

  ServiceManager* sm = &managers[managers_len];
  managers_len++;

  memset(sm, 0, sizeof(ServiceManager));
  sm->name = strdup(name);
  memcpy(&sm->config, config, sizeof(ServiceManagerConfig));
  sm->data = data;
  sm->free_data = free_data;
  InitServiceManager(sm);
  return sm;
}

const char* GetServiceManagerName(ServiceManager* sm) {
  return sm ? sm->name : NULL;
}

bool StartService(ServiceManager* sm, const char* name) {
  bool result = false;
  if (!sm || !name)
    goto finished;

  if (!sm->config.start) {
    result = true;
    goto finished;
  }

  result = sm->config.start(name, sm->data);
finished:
  return result;
}

bool StopService(ServiceManager* sm, const char* name) {
  bool result = false;
  if (!sm || !name)
    goto finished;

  if (!sm->config.stop) {
    result = true;
    goto finished;
  }

  result = sm->config.stop(name, sm->data);
finished:
  return result;
}

bool RestartService(ServiceManager* sm, const char* name) {
  bool result = false;
  if (!sm || !name)
    goto finished;

  if (!sm->config.restart) {
    result = true;
    goto finished;
  }

  result = sm->config.restart(name, sm->data);
finished:
  return result;
}

bool ReloadService(ServiceManager* sm, const char* name) {
  bool result = false;
  if (!sm || !name)
    goto finished;

  if (!sm->config.reload) {
    result = true;
    goto finished;
  }

  result = sm->config.reload(name, sm->data);
finished:
  return result;
}

bool StatusService(ServiceManager* sm, const char* name) {
  bool result = false;
  if (!sm || !name)
    goto finished;

  if (!sm->config.status) {
    result = true;
    goto finished;
  }

  result = sm->config.status(name, sm->data);
finished:
  return result;
}

void VisitAllServiceManagers(VisitServiceManagerFn fn, void* data) {
  ASSERT(fn);
  if (!managers || managers_len == 0)
    return;

  for (size_t i = 0; i < managers_len; i++) {
    ServiceManager* sm = &managers[i];
    ASSERT(sm);
    if (!fn(i, sm, data))
      return;
  }
}

size_t GetTotalNumberOfServiceManagers() {
  return managers_len;
}

ServiceManager* GetServiceManagerAt(const size_t idx) {
  if (!managers || idx > managers_len)
    return NULL;

  return &managers[idx];
}

ServiceManager* FindServiceManager(const char* name) {
  if (!managers || managers_len == 0 || !name)
    return NULL;

  for (size_t i = 0; i < managers_len; i++) {
    ServiceManager* sm = &managers[i];
    ASSERT(sm);
    if (strcmp(sm->name, name) == 0)
      return sm;
  }

  return NULL;
}

static inline void FreeServiceManager(ServiceManager* sm) {
  if (!sm)
    return;

  DeInitServiceManager(sm);

  if (sm->name)
    free(sm->name);

  if (sm->data && sm->free_data)
    sm->free_data(sm->data);
}

void FreeAllServiceManagers() {
  if (!managers || managers_len == 0)
    return;

  for (size_t i = 0; i < managers_len; i++)
    FreeServiceManager(&managers[i]);

  free(managers);
  managers = NULL;
  managers_len = managers_cap = 0;
}
