#define _DEFAULT_SOURCE
#include "hypha/glob.h"

#include <dirent.h>
#include <fnmatch.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "hypha/log.h"

#ifndef FNM_CASEFOLD
#ifdef FNM_IGNORECASE
#define FNM_CASEFOLD FNM_IGNORECASE
#else
#define FNM_CASEFOLD 0
#endif  // FNM_IGNORECASE
#endif  // FNM_CASEFOLD

static inline bool PushResult(Glob* glob, const char* p) {
  if (glob->paths_len >= glob->paths_cap) {
    size_t new_cap = glob->paths_cap == 0 ? 8 : glob->paths_cap * 2;
    char** new_paths = realloc(glob->paths, new_cap * sizeof(char*));
    if (!new_paths)
      return false;
    glob->paths = new_paths;
    glob->paths_cap = new_cap;
  }

  glob->paths[glob->paths_len] = strdup(p);
  if (!glob->paths[glob->paths_len])
    return false;

  glob->paths_len++;
  return true;
}

bool GlobFiles(const char* root, const char* pattern, Glob* glob, const bool recursive) {
  DIR* dir = opendir(root);
  if (!dir)
    return false;

  char full_path[PATH_MAX];
  struct dirent* entry = NULL;
  bool overall_success = true;

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    if (entry->d_name[0] == '.')
      continue;

    if (snprintf(full_path, PATH_MAX, "%s/%s", root, entry->d_name) >= PATH_MAX) {
      continue;
    }

    if (entry->d_type == DT_DIR) {
      if (recursive) {
        if (!GlobFiles(full_path, pattern, glob, recursive)) {
          overall_success = false;
          break;
        }
      }
    } else if (entry->d_type == DT_REG) {
      if (fnmatch("*.jsonnet", entry->d_name, FNM_CASEFOLD) == 0 ||
          fnmatch("*.json", entry->d_name, FNM_CASEFOLD) == 0 || fnmatch("*.yaml", entry->d_name, FNM_CASEFOLD) == 0 ||
          fnmatch("*.yml", entry->d_name, FNM_CASEFOLD) == 0) {
        if (!PushResult(glob, full_path)) {
          overall_success = false;
          break;
        }
      }
    }
  }

  closedir(dir);
  return overall_success;
}
