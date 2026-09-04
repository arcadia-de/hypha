#include "hypha/process.h"

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "hypha/log.h"

static inline void FlushAccumulatedLines(Process* p, char* accumulator, int is_stderr) {
  char* newline_pos = NULL;
  char* search_ptr = accumulator;

  while ((newline_pos = strchr(search_ptr, '\n')) != NULL) {
    *newline_pos = '\0';

    if (is_stderr) {
      if (p->err)
        p->err(p, search_ptr);
    } else {
      if (p->out)
        p->out(p, search_ptr);
    }

    search_ptr = newline_pos + 1;
  }

  if (*search_ptr != '\0') {
    memmove(accumulator, search_ptr, strlen(search_ptr) + 1);
  } else {
    accumulator[0] = '\0';
  }
}

static inline void InitRootEnv(const char** in_env, const size_t in_env_size, char*** out_env, size_t* out_env_size) {
  static const size_t kTotalNumberOfDefaultVars = 3;
  const uint64_t total_env_vars = kTotalNumberOfDefaultVars + in_env_size + 1;
  char** env = (char**)malloc(sizeof(char*) * total_env_vars);
  env[0] = "PATH=/usr/bin:/bin";
  env[1] = "HOME=/root";
  env[2] = "USER=root";
  for (uint64_t i = 0; i < in_env_size; i++)
    env[i + kTotalNumberOfDefaultVars] = (char*)in_env[i];
  env[in_env_size + kTotalNumberOfDefaultVars] = NULL;

  (*out_env) = env;
  (*out_env_size) = total_env_vars;
}

static inline void InitDefaultEnv(const char** in_env, const size_t in_env_size, char*** out_env,
                                  size_t* out_env_size) {
  static const size_t kTotalNumberOfDefaultVars = 1;
  const uint64_t total_env_vars = kTotalNumberOfDefaultVars + in_env_size + 1;
  char** env = (char**)malloc(sizeof(char*) * total_env_vars);
  env[0] = "PATH=/usr/bin:/bin:/usr/local/bin";
  for (uint64_t i = 0; i < in_env_size; i++)
    env[i + kTotalNumberOfDefaultVars] = (char*)in_env[i];
  env[in_env_size + kTotalNumberOfDefaultVars] = NULL;

  (*out_env) = env;
  (*out_env_size) = total_env_vars;
}

static inline void InitArgs(const char* bin, const char** in_args, const size_t in_num_args, char*** out_args,
                            size_t* out_num_args) {
  static const size_t kTotalNumberOfDefaultArgs = 1;
  const size_t total_args = kTotalNumberOfDefaultArgs + in_num_args + 1;
  char** args = (char**)malloc(sizeof(char*) * total_args);
  args[0] = (char*)bin;
  for (size_t i = 0; i < in_num_args; i++)
    args[i + kTotalNumberOfDefaultArgs] = (char*)in_args[i];
  args[in_num_args + kTotalNumberOfDefaultArgs] = NULL;

  (*out_args) = args;
  (*out_num_args) = total_args;
}

int ExecProcess(Process* p) {
  if (!p)
    return -1;

  char** args = NULL;
  size_t args_size = 0;
  InitArgs(p->bin, p->args, p->num_args, &args, &args_size);

  char** env = NULL;
  size_t env_size = 0;
  if (p->root) {
    if (geteuid() != 0) {
      LOG_ERROR("this binary must be owned by root and have the SUID bit set.");
      return -1;
    }

    if (setuid(0) != 0) {
      LOG_FATAL("setuid failed");
      return -1;
    }

    InitRootEnv(p->env_variables, p->num_env_variables, &env, &env_size);
  } else {
    InitDefaultEnv(p->env_variables, p->num_env_variables, &env, &env_size);
  }

  int stdout_pipe[2];
  int stderr_pipe[2];

  if (pipe(stdout_pipe) < 0)
    goto failed0;
  if (pipe(stderr_pipe) < 0) {
    goto failed1;
  }

  pid_t pid = fork();
  if (pid < 0)
    goto failed2;

  if (pid == 0) {
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    dup2(stdout_pipe[1], STDOUT_FILENO);
    dup2(stderr_pipe[1], STDERR_FILENO);

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    clock_gettime(CLOCK_REALTIME, &p->start);
    execve(p->bin, args, env);
    clock_gettime(CLOCK_REALTIME, &p->finish);
    free(env);
    free(args);
    exit(127);
  }

  close(stdout_pipe[1]);
  close(stderr_pipe[1]);

  char buffer[HYPHA_PROCESS_BUFFER_SIZE];
  char stdout_accumulator[4096] = {0};
  char stderr_accumulator[4096] = {0};

  struct pollfd fds[2];
  fds[0].fd = stdout_pipe[0];
  fds[0].events = POLLIN;

  fds[1].fd = stderr_pipe[0];
  fds[1].events = POLLIN;

  int active_pipes = 2;

  while (active_pipes > 0) {
    int poll_count = poll(fds, 2, p->timeout);
    if (poll_count < 0) {
      perror("Poll error during process tracking");
      break;
    }

    if (poll_count == 0) {
      DLOG_ERROR("process timed out.");
      kill(pid, SIGKILL);
      int status = 0;
      waitpid(pid, &status, 0);
      free(env);
      free(args);
      return -1;
    }

    for (int i = 0; i < 2; i++) {
      if (fds[i].fd == -1)
        continue;

      if (fds[i].revents & POLLIN) {
        ssize_t bytes_read = read(fds[i].fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
          buffer[bytes_read] = '\0';
          if (i == 0) {
            size_t current_len = strlen(stdout_accumulator);
            strncat(stdout_accumulator, buffer, sizeof(stdout_accumulator) - current_len - 1);
            FlushAccumulatedLines(p, stdout_accumulator, 0);
          } else {
            size_t current_len = strlen(stderr_accumulator);
            strncat(stderr_accumulator, buffer, sizeof(stderr_accumulator) - current_len - 1);
            FlushAccumulatedLines(p, stderr_accumulator, 1);
          }
        } else if (bytes_read == 0) {
          close(fds[i].fd);
          fds[i].fd = -1;
          active_pipes--;
          continue;
        }
      }

      if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
        close(fds[i].fd);
        fds[i].fd = -1;
        active_pipes--;
      }
    }
  }

  if (stdout_accumulator[0] != '\0')
    LOG_INFO("%s", stdout_accumulator);
  if (stderr_accumulator[0] != '\0')
    LOG_ERROR("%s", stderr_accumulator);

  free(env);
  free(args);
  int status = 0;
  // NOTE: was WNOHANG. Both output pipes have already hit EOF by this point (the poll loop
  // above only exits once active_pipes reaches 0), which happens when the child exits and
  // its write ends close -- so the child is already a zombie or about to become one, and
  // blocking here is safe, not a hang risk. WNOHANG can return 0 (no state change available
  // yet) in the narrow window before the kernel finishes tearing the child down, leaving
  // `status` at its initialized value of 0 -- and WIFEXITED(0) is true with
  // WEXITSTATUS(0) == 0, so a process that actually exited non-zero (or was killed by a
  // signal) could be silently reported as a clean exit. Caught via a Task `check` command
  // (`/usr/bin/false`, exit 1) intermittently reporting success.
  waitpid(pid, &status, 0);
  clock_gettime(CLOCK_REALTIME, &p->start);

  if (p->on_finished)
    p->on_finished(p);

  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
failed2:
  close(stderr_pipe[0]);
  close(stderr_pipe[1]);
failed1:
  close(stdout_pipe[0]);
  close(stdout_pipe[1]);
failed0:
  return -1;
}

static inline void OnWhich(Process* p, const char* message) {
  p->data = (void*)strdup(message);
}

bool ExecWhich(const char* bin, char** result) {
  Process proc;
  memset(&proc, 0, sizeof(Process));
  proc.bin = HYPHA_WHICH_PATH;

  const char* args[1];
  args[0] = bin;

  proc.args = args;
  proc.num_args = 1;
  proc.data = NULL;
  proc.out = &OnWhich;
  proc.timeout = 5000;

  const int status = ExecProcess(&proc);
  if (status != 0) {
    (*result) = NULL;
    return false;
  }

  (*result) = (char*)proc.data;
  return true;
}
