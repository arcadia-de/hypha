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
      if (p->err) {
        p->err(p, search_ptr);
      }
    } else {
      if (p->out) {
        p->out(p, search_ptr);
      }
    }

    search_ptr = newline_pos + 1;
  }

  if (*search_ptr != '\0') {
    memmove(accumulator, search_ptr, strlen(search_ptr) + 1);
  } else {
    accumulator[0] = '\0';
  }
}

int ExecProcess(Process* p) {
  if (!p)
    return -1;

  if (p->root) {
    if (geteuid() != 0) {
      LOG_ERROR("this binary must be owned by root and have the SUID bit set.");
      return -1;
    }

    if (setuid(0) != 0) {
      LOG_FATAL("setuid failed");
      return -1;
    }
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

    char** args = (char**)malloc(sizeof(char*) * (2 + p->num_args));
    args[0] = (char*)p->bin;
    for (uint64_t i = 0; i < p->num_args; i++)
      args[i + 1] = (char*)p->args[i];
    args[p->num_args + 1] = NULL;

    // LOG_INFO("args %d:", (1 + p->num_args));
    // for (uint64_t i = 0; i < (1 + p->num_args); i++)
    //   LOG_INFO(" - %s", args[i]);

    const uint64_t total_env_vars = (p->root ? 4 : 2) + p->num_env_variables;
    char** env = (char**)malloc(sizeof(char*) * total_env_vars);
    if (p->root) {
      env[0] = "PATH=/usr/bin:/bin";
    } else {
      env[0] = "PATH=/usr/bin:/bin:/usr/local/bin";
    }
    env[1] = "HOME=/root";
    env[2] = "USER=root";
    for (uint64_t i = 0; i < p->num_env_variables; i++)
      env[i + (p->root ? 3 : 1)] = (char*)p->env_variables[i];
    env[p->num_env_variables + (p->root ? 3 : 1)] = NULL;

    // LOG_INFO("env %d:", (1 + p->num_env_variables));
    // for (uint64_t i = 0; i < (1 + p->num_env_variables); i++)
    //   LOG_INFO(" - %s", env[i]);

    execve(p->bin, args, env);
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
    int poll_count = poll(fds, 2, -1);
    if (poll_count < 0) {
      perror("Poll error during process tracking");
      break;
    }

    for (int i = 0; i < 2; i++) {
      if (fds[i].fd == -1)
        continue;

      if (fds[i].revents & POLLIN) {
        ssize_t bytes_read = read(fds[i].fd, buffer, sizeof(buffer) - 1);

        if (bytes_read > 0) {
          buffer[bytes_read] = '\0';
          if (i == 0) {
            strcat(stdout_accumulator, buffer);
            FlushAccumulatedLines(p, stdout_accumulator, 0);
          } else {
            strcat(stderr_accumulator, buffer);
            FlushAccumulatedLines(p, stderr_accumulator, 1);
          }
        } else if (bytes_read == 0) {
          close(fds[i].fd);
          fds[i].fd = -1;
          active_pipes--;
        }
      }

      if (fds[i].revents & (POLLHUP | POLLERR)) {
        if (fds[i].fd != -1) {
          close(fds[i].fd);
          fds[i].fd = -1;
          active_pipes--;
        }
      }
    }
  }

  if (stdout_accumulator[0] != '\0')
    LOG_INFO("%s", stdout_accumulator);
  if (stderr_accumulator[0] != '\0')
    LOG_ERROR("%s", stderr_accumulator);

  int status = 0;
  waitpid(pid, &status, 0);
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

  const int status = ExecProcess(&proc);
  if (status != 0) {
    (*result) = NULL;
    return false;
  }

  (*result) = (char*)proc.data;
  return true;
}
