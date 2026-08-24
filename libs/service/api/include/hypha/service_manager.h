#ifndef HYPHA_SERVICE_MANAGER_H
#define HYPHA_SERVICE_MANAGER_H

#include <stdint.h>
#include <stdlib.h>

typedef void (*ServiceManagerInitFn)(void* data);

typedef void (*ServiceManagerDeInitFn)(void* data);

typedef bool (*ServiceManagerStartFn)(const char* name, void* data);

typedef bool (*ServiceManagerStopFn)(const char* name, void* data);

typedef bool (*ServiceManagerRestartFn)(const char* name, void* data);

typedef bool (*ServiceManagerReloadFn)(const char* name, void* data);

typedef bool (*ServiceManagerStatusFn)(const char* name, void* data);

typedef struct {
  ServiceManagerInitFn init;
  ServiceManagerDeInitFn deinit;
  ServiceManagerStartFn start;
  ServiceManagerStopFn stop;
  ServiceManagerRestartFn restart;
  ServiceManagerReloadFn reload;
  ServiceManagerStatusFn status;
} ServiceManagerConfig;

typedef struct _ServiceManager ServiceManager;

ServiceManager* NewServiceManager(const char* name, const ServiceManagerConfig* config, void* data,
                                  void (*free_data)(void*));
size_t GetTotalNumberOfServiceManagers();
ServiceManager* GetServiceManagerAt(const size_t idx);
ServiceManager* FindServiceManager(const char* name);
const char* GetServiceManagerName(ServiceManager* sm);
bool StartService(ServiceManager* sm, const char* name);
bool StopService(ServiceManager* sm, const char* name);
bool RestartService(ServiceManager* sm, const char* name);
bool ReloadService(ServiceManager* sm, const char* name);
bool StatusService(ServiceManager* sm, const char* name);

typedef bool (*VisitServiceManagerFn)(uint64_t idx, ServiceManager* sm, void* data);

void VisitAllServiceManagers(VisitServiceManagerFn fn, void* data);
void FreeAllServiceManagers();

#endif  // HYPHA_SERVICE_MANAGER_H
