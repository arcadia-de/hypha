#ifndef HYPHA_PROCESS_H
#define HYPHA_PROCESS_H

#include <stdint.h>
#include <time.h>

#ifndef HYPHA_PROCESS_BUFFER_SIZE
#define HYPHA_PROCESS_BUFFER_SIZE 512
#endif  // HYPHA_PROCESS_BUFFER_SIZE

typedef struct _Process Process;

typedef void (*ProcessLogFn)(Process* p, const char* message);

typedef void (*ProcessCallback)(Process* p);

struct _Process {
  bool root;
  const char* bin;
  int code;
  int timeout;

  const char** args;
  uint64_t num_args;

  const char** env_variables;
  uint64_t num_env_variables;

  void* data;
  ProcessLogFn out;
  ProcessLogFn err;
  ProcessCallback on_finished;

  // ╭───────────╮
  // │ Telemetry │
  // ╰───────────╯
  struct timespec start;
  struct timespec finish;
};

int ExecProcess(Process* p);

#ifndef HYPHA_WHICH_PATH
#define HYPHA_WHICH_PATH "/bin/which"
#endif  // HYPHA_WHICH_PATH

bool ExecWhich(const char* bin, char** result);

#endif  // HYPHA_PROCESS_H
