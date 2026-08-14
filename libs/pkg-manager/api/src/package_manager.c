#include "hypha/package_manager.h"

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/log.h"
#include "hypha/process.h"

struct _PackageManager {
  char* name;
  char* bin;
  PackageManagerConfig config;
  void* data;
  void (*free_data)(void*);
};

PackageManager* managers[64];
static uint64_t num_managers = 0;

PackageManager* NewPackageManager(const char* name, const char* bin, const PackageManagerConfig* config, void* data,
                                  void (*free_data)(void*)) {
  ASSERT(name);
  PackageManager* m = (PackageManager*)malloc(sizeof(PackageManager));
  if (m) {
    memset(m, 0, sizeof(PackageManager));
    memcpy(&m->config, config, sizeof(PackageManagerConfig));
    m->name = strdup(name);

    if (!bin) {
      char* p = NULL;
      if (!ExecWhich(name, &p)) {
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "/usr/bin/%s", name);
        m->bin = strdup(path);
      } else {
        m->bin = p;
      }

    } else {
      m->bin = strdup(bin);
    }

    m->data = data;
    m->free_data = free_data;
    managers[num_managers] = m;
    num_managers++;
    LOG_DEBUG("created %s package manager", name);
  }

  return m;
}

const char* GetPackageManagerName(const PackageManager* pm) {
  return pm ? pm->name : NULL;
}

const char* GetPackageManagerPath(const PackageManager* pm) {
  return pm ? pm->bin : NULL;
}

uint64_t GetNumberOfPackageManagers() {
  return num_managers;
}

PackageManager* GetPackageManagerAt(const uint64_t idx) {
  if (idx >= num_managers)
    return NULL;
  return managers[idx];
}

bool IsPackageManagerNamed(const PackageManager* pm, const char* name) {
  return pm && name && strcmp(pm->name, name) == 0;
}

PackageManager* FindPackageManager(const char* name) {
  for (uint64_t i = 0; i < num_managers; i++) {
    PackageManager* m = managers[i];
    if (IsPackageManagerNamed(m, name))
      return m;
  }

  return NULL;
}

void FreePackageManager(PackageManager* rhs) {
  if (!rhs)
    return;

  if (rhs->data && rhs->free_data)
    rhs->free_data(rhs->data);

  free(rhs->name);
}

PackageStatus PackageManagerStatus(PackageManager* mgr, const char* pkg) {
  PackageStatus status = kPackageSkipped;
  if (!mgr || !pkg || !mgr->config.status)
    goto finished;

  status = mgr->config.status(mgr, pkg, mgr->data);
finished:
  return status;
}

PackageStatus PackageManagerInstall(PackageManager* mgr, const char* pkg) {
  PackageStatus status = kPackageSkipped;
  if (!mgr || !pkg || !mgr->config.install)
    goto finished;

  status = mgr->config.install(mgr, pkg, mgr->data);
finished:
  return status;
}

PackageStatus PackageManagerUninstall(PackageManager* mgr, const char* pkg) {
  PackageStatus status = kPackageSkipped;
  if (!mgr || !pkg || !mgr->config.uninstall)
    goto finished;

  status = mgr->config.uninstall(mgr, pkg, mgr->data);
finished:
  return status;
}

int ExecPackageManager(PackageManager* mgr, const char** args, const uint64_t num_args, const bool root) {
  Process proc;
  memset(&proc, 0, sizeof(Process));
  proc.bin = GetPackageManagerPath(mgr);
  proc.num_args = num_args;
  proc.args = args;
  proc.root = root;
  return ExecProcess(&proc);
}
