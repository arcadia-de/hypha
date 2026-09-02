#include "hypha/platform.h"

#include <bits/posix1_lim.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

char* GetOS() {
#if defined(_WIN32) || defined(_WIN64)
  return strdup("windows");
#elif defined(__APPLE__) || defined(__MACH__)
  return strdup("darwin");
#elif defined(__linux__)
  return strdup("linux");
#elif defined(__FreeBSD__)
  return strdup("freebsd");
#elif defined(__unix__) || defined(__unix)
  return strdup("unix");
#else
  return strdup("Unknown");
#endif
}

char* GetHostname() {
  char hostname[HOST_NAME_MAX + 1];
  if (gethostname(hostname, HOST_NAME_MAX + 1) != 0)
    return NULL;
  return strdup(hostname);
}

char* GetUsername() {
#if defined(_WIN32) || defined(_WIN64)
  return strdup(getenv("USERNAME"));
#else
  return strdup(getenv("USER"));
#endif
}

char* GetArch() {
#if defined(__x86_64__) || defined(_M_X64)
  return strdup("x86_64");
#elif defined(__i386__) || defined(_M_IX86)
  return strdup("x86");
#elif defined(__aarch64__) || defined(_M_ARM64)
  return strdup("arm64");
#elif defined(__arm__) || defined(_M_ARM)
  return strdup("arm");
#else
  return strdup("unknown");
#endif
}

#define OS_RELEASE_FILENAME "/etc/os-release"

char* GetDistro() {
  FILE* fp = fopen(OS_RELEASE_FILENAME, "r");
  if (fp == NULL)
    goto finished;

  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
      char* start = strchr(line, '"');
      char* end = strrchr(line, '"');
      if (start && end && start != end) {
        *end = '\0';
        fclose(fp);
        return strdup(start + 1);
      }
    }
  }

finished:
  if (fp)
    fclose(fp);
  return strdup("Unknown");
}

// LUA_FN(has) {
//   const char* pkg = luaL_checkstring(L, 1);
//
//   char* result = NULL;
//   if (!ExecWhich(pkg, &result))
//     return false;
//
//   lua_pushboolean(L, result != NULL);
//   return 1;
// }
