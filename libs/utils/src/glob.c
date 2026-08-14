#define _DEFAULT_SOURCE
#include "hypha/glob.h"

#include <dirent.h>
#include <fnmatch.h>
#include <lauxlib.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef FNM_CASEFOLD
#ifdef FNM_IGNORECASE

#define FNM_CASEFOLD FNM_IGNORECASE

#else

#define FNM_CASEFOLD 0

#endif  // FNM_IGNORECASE
#endif  // FNM_CASEFOLD

static inline bool PushResult(Glob* glob, const char* p) {
  if (glob->paths_len >= glob->paths_cap) {
    glob->paths_cap = glob->paths_cap == 0 ? 8 : glob->paths_cap * 2;
    char** new_paths = realloc(glob->paths, glob->paths_cap * sizeof(char*));
    if (!new_paths)
      return false;

    glob->paths = new_paths;
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
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if (entry->d_name[0] == '.')
      continue;

    snprintf(full_path, PATH_MAX, "%s/%s", root, entry->d_name);
    if (entry->d_type == DT_DIR) {
      if (recursive) {
        if (!GlobFiles(full_path, pattern, glob, recursive))
          return false;
      }
    } else if (entry->d_type == DT_REG) {
      if (fnmatch(pattern, entry->d_name, FNM_CASEFOLD) == 0)
        PushResult(glob, full_path);
    }
  }

  closedir(dir);
  return false;
}
