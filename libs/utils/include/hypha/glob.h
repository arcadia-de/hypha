#ifndef HYPHA_GLOB_H
#define HYPHA_GLOB_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char** paths;
  size_t paths_len;
  size_t paths_cap;
} Glob;

static inline void InitGlob(Glob* gl, const size_t init_cap) {
  if (!gl)
    return;

  memset(gl, 0, sizeof(Glob));
  gl->paths = NULL;
  gl->paths_len = gl->paths_cap = 0;

  if (init_cap > 0) {
    const size_t total_size = sizeof(char*) * init_cap;
    char** new_paths = (char**)malloc(total_size);
    if (!new_paths)
      return;

    gl->paths = new_paths;
    gl->paths_len = 0;
    gl->paths_cap = init_cap;
  }
}

static inline void ClearGlob(Glob* gl) {
  if (!gl)
    return;

  if (gl->paths) {
    for (size_t i = 0; i < gl->paths_len; i++) {
      if (gl->paths[i]) {
        // Clear out data tracks to ensure no ghost characters remain
        memset(gl->paths[i], 0, strlen(gl->paths[i]));
        free(gl->paths[i]);
        gl->paths[i] = NULL;
      }
    }
    free(gl->paths);
    gl->paths = NULL;
  }
  gl->paths_len = 0;
  gl->paths_cap = 0;
}

bool GlobFiles(const char* dir, const char* pattern, Glob* glob, const bool recursive);

#endif  // HYPHA_GLOB_H
