#include "systemd.h"

#include "hypha/log.h"

static const char* kName = "SystemD";
static const ServiceManagerConfig kConfig = {};
void InitSystemDServiceManager() {
  ServiceManager* sm = NewServiceManager(kName, &kConfig, NULL, NULL);
  LOG_ERROR_IF(!sm, "failed to create `%s` service manager", kName);
}
