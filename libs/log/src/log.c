#include "hypha/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#include "ansi.h"

thread_local const char* current_resource_id = NULL;
thread_local const char* current_resource_kind = NULL;

void SetLogResourceContext(const char* id, const char* kind) {
  current_resource_id = id;
  current_resource_kind = kind;
}

void ClearLogResourceContext() {
  return SetLogResourceContext(NULL, NULL);
}

static inline void LogPrefix(FILE* stream, const char* file, const int line, const char* level) {
  fprintf(stream, "[%s] [%s:%d] [%s] ", "", file, line, level);
  if (current_resource_id && current_resource_kind)
    fprintf(stream, "[%s (%s)] ", current_resource_id, current_resource_kind);
}

#define _LOG_INFO_PREFIX(Stream, File, Line)    LogPrefix(Stream, File, Line, ANSI_BOLD ANSI_CYAN "INFO" ANSI_RESET)

#define _LOG_SUCCESS_PREFIX(Stream, File, Line) LogPrefix(Stream, File, Line, ANSI_BOLD ANSI_GREEN "SUCCESS" ANSI_RESET)

#define _LOG_DEBUG_PREFIX(Stream, File, Line)   LogPrefix(Stream, File, Line, ANSI_BOLD ANSI_PURPLE "DEBUG" ANSI_RESET)

#define _LOG_WARN_PREFIX(Stream, File, Line)    LogPrefix(Stream, File, Line, ANSI_BOLD ANSI_YELLOW "WARN" ANSI_RESET)

#define _LOG_ERROR_PREFIX(Stream, File, Line)   LogPrefix(Stream, File, Line, ANSI_BOLD ANSI_RED "ERROR" ANSI_RESET)

#define _LOG_FATAL_PREFIX(Stream, File, Line)   LogPrefix(Stream, File, Line, ANSI_BOLD ANSI_RED "FATAL" ANSI_RESET)

#define _LOG_ARGS(Stream, Format) \
  va_list args;                   \
  va_start(args, Format);         \
  vfprintf(Stream, Format, args); \
  va_end(args);

void LogInfo(const char* file, const int line, const char* fmt, ...) {
  _LOG_INFO_PREFIX(stdout, file, line);
  _LOG_ARGS(stdout, fmt);
  fprintf(stdout, "\n");
}

void LogSuccess(const char* file, const int line, const char* fmt, ...) {
  _LOG_SUCCESS_PREFIX(stdout, file, line);
  _LOG_ARGS(stdout, fmt);
  fprintf(stdout, "\n");
}

void LogDebug(const char* file, const int line, const char* fmt, ...) {
  _LOG_DEBUG_PREFIX(stdout, file, line);
  _LOG_ARGS(stdout, fmt);
  fprintf(stdout, "\n");
}

void LogWarn(const char* file, const int line, const char* fmt, ...) {
  _LOG_WARN_PREFIX(stdout, file, line);
  _LOG_ARGS(stdout, fmt);
  fprintf(stdout, "\n");
}

void LogError(const char* file, const int line, const char* fmt, ...) {
  _LOG_ERROR_PREFIX(stderr, file, line);
  _LOG_ARGS(stderr, fmt);
  fprintf(stderr, "\n");
}

void LogFatal(const char* file, const int line, const char* fmt, ...) {
  _LOG_FATAL_PREFIX(stderr, file, line);
  _LOG_ARGS(stderr, fmt);
  fprintf(stderr, "\n");
  exit(1);
}
