#include "exec_spec.h"

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/assertions.h"
#include "hypha/log.h"

static const char* kCommandField = "command";
static inline void GetSpecCommand(json_t* doc, char*** result, size_t* result_len) {
  ASSERT(doc);
  json_t* command = json_object_get(doc, kCommandField);
  if (!command || !json_is_array(command))
    return;

  size_t len = json_array_size(command);
  char** argv = (char**)malloc(sizeof(char*) * (len + 1));
  LOG_FATAL_IF(!argv, "failed to allocate argv");

  for (size_t i = 0; i < len; i++) {
    json_t* arg = json_array_get(command, i);
    LOG_FATAL_IF(!arg || !json_is_string(arg), "failed to get arg %zu for command", i);
    argv[i] = strdup(json_string_value(arg));
  }
  argv[len] = NULL;

  (*result) = argv;
  (*result_len) = len;
}

static const char kWorkdirField[] = "workdir";
static inline void GetSpecWorkdir(json_t* doc, char** result) {
  ASSERT(doc);
  json_t* workdir = json_object_get(doc, kWorkdirField);
  if (!workdir || !json_is_string(workdir)) {
    (*result) = NULL;
    return;
  }

  (*result) = strdup(json_string_value(workdir));
}

static const char kShellField[] = "shell";
static inline void GetSpecShell(json_t* doc, char** result, bool* use_default) {
  ASSERT(doc);
  (*result) = NULL;
  (*use_default) = false;

  json_t* shell = json_object_get(doc, kShellField);
  if (!shell)
    return;

  if (json_is_string(shell)) {
    (*result) = strdup(json_string_value(shell));
    return;
  }

  if (json_is_true(shell)) {
    (*use_default) = true;
    return;
  }
}

void ParseExecSpec(json_t* doc, ExecSpec* spec) {
  ASSERT(doc);

  GetSpecWorkdir(doc, &spec->workdir);
  GetSpecShell(doc, &spec->shell, &spec->use_default_shell);
  GetSpecCommand(doc, &spec->argv, &spec->argv_len);
}

#ifndef HYPHA_DEFAULT_SHELL_ENV
#define HYPHA_DEFAULT_SHELL_ENV "HYPHA_DEFAULT_SHELL"
#endif  // HYPHA_DEFAULT_SHELL_ENV

#ifndef HYPHA_FALLBACK_SHELL
#define HYPHA_FALLBACK_SHELL "/bin/sh"
#endif  // HYPHA_FALLBACK_SHELL

const char* ResolveShellPath(const ExecSpec* spec) {
  ASSERT(spec);

  if (spec->shell)
    return spec->shell;

  if (!spec->use_default_shell)
    return NULL;

  const char* configured = getenv(HYPHA_DEFAULT_SHELL_ENV);
  if (configured && configured[0] != '\0')
    return configured;

  const char* env_shell = getenv("SHELL");
  if (env_shell && env_shell[0] != '\0')
    return env_shell;

  return HYPHA_FALLBACK_SHELL;
}
