#ifndef HYPHA_LOG_H
#define HYPHA_LOG_H

void LogInfo(const char* file, const int line, const char* fmt, ...);
void LogSuccess(const char* file, const int line, const char* fmt, ...);
void LogDebug(const char* file, const int line, const char* fmt, ...);
void LogWarn(const char* file, const int line, const char* fmt, ...);
void LogError(const char* file, const int line, const char* fmt, ...);
void LogFatal(const char* file, const int line, const char* fmt, ...);

void SetLogResourceContext(const char* id, const char* kind);
void ClearLogResourceContext();

#define LOG_INFO(fmt, ...)    LogInfo(__FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)   LogDebug(__FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_SUCCESS(fmt, ...) LogSuccess(__FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)    LogWarn(__FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   LogError(__FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...)   LogFatal(__FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_IF(Level, Cond, Fmt, ...)  \
  ({                                   \
    if ((Cond))                        \
      LOG_##Level(Fmt, ##__VA_ARGS__); \
  })

#define LOG_INFO_IF(Cond, fmt, ...)    LOG_IF(INFO, Cond, fmt, ##__VA_ARGS__)
#define LOG_DEBUG_IF(Cond, fmt, ...)   LOG_IF(DEBUG, Cond, fmt, ##__VA_ARGS__)
#define LOG_SUCCESS_IF(Cond, fmt, ...) LOG_IF(SUCCESS, Cond, fmt, ##__VA_ARGS__)
#define LOG_WARN_IF(Cond, fmt, ...)    LOG_IF(WARN, Cond, fmt, ##__VA_ARGS__)
#define LOG_ERROR_IF(Cond, fmt, ...)   LOG_IF(ERROR, Cond, fmt, ##__VA_ARGS__)
#define LOG_FATAL_IF(Cond, fmt, ...)   LOG_IF(FATAL, Cond, fmt, ##__VA_ARGS__)

#ifdef HYPHA_DEBUG

#define DLOG_INFO(fmt, ...)    LOG_INFO(fmt, ##__VA_ARGS__)
#define DLOG_SUCCESS(fmt, ...) LOG_SUCCESS(fmt, ##__VA_ARGS__)
#define DLOG_WARN(fmt, ...)    LOG_WARN(fmt, ##__VA_ARGS__)
#define DLOG_ERROR(fmt, ...)   LOG_ERROR(fmt, ##__VA_ARGS__)
#define DLOG_FATAL(fmt, ...)   LOG_FATAL(fmt, ##__VA_ARGS__)
#define DLOG_DEBUG(fmt, ...)   LOG_DEBUG(fmt, ##__VA_ARGS__)

#define DLOG_IF(Level, Cond, Fmt, ...)  \
  ({                                    \
    if ((Cond))                         \
      DLOG_##Level(Fmt, ##__VA_ARGS__); \
  })

#define DLOG_INFO_IF(Cond, fmt, ...)    DLOG_IF(INFO, Cond, fmt, ##__VA_ARGS__)
#define DLOG_DEBUG_IF(Cond, fmt, ...)   DLOG_IF(DEBUG, Cond, fmt, ##__VA_ARGS__)
#define DLOG_SUCCESS_IF(Cond, fmt, ...) DLOG_IF(SUCCESS, Cond, fmt, ##__VA_ARGS__)
#define DLOG_WARN_IF(Cond, fmt, ...)    DLOG_IF(WARN, Cond, fmt, ##__VA_ARGS__)
#define DLOG_ERROR_IF(Cond, fmt, ...)   DLOG_IF(ERROR, Cond, fmt, ##__VA_ARGS__)
#define DLOG_FATAL_IF(Cond, fmt, ...)   DLOG_IF(FATAL, Cond, fmt, ##__VA_ARGS__)

#else

#define DLOG_INFO(fmt, ...)
#define DLOG_SUCCESS(fmt, ...)
#define DLOG_WARN(fmt, ...)
#define DLOG_ERROR(fmt, ...)
#define DLOG_FATAL(fmt, ...)
#define DLOG_DEBUG(fmt, ...)

#define DLOG_INFO_IF(Cond, fmt, ...)
#define DLOG_DEBUG_IF(Cond, fmt, ...)
#define DLOG_SUCCESS_IF(Cond, fmt, ...)
#define DLOG_WARN_IF(Cond, fmt, ...)
#define DLOG_ERROR_IF(Cond, fmt, ...)
#define DLOG_FATAL_IF(Cond, fmt, ...)

#endif

#endif  // HYPHA_LOG_H
