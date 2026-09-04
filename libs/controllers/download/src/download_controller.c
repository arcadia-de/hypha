#include "hypha/download_controller.h"

#include <ctype.h>
#include <curl/curl.h>
#include <errno.h>
#include <jansson.h>
#include <linux/limits.h>
#include <sodium.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/controller_status.h"
#include "hypha/delta_log.h"
#include "hypha/download_spec.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/reason.h"
#include "hypha/validation_log.h"

static inline void InitController(void* data) {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  if (sodium_init() < 0)
    LOG_ERROR("failed to initialize libsodium");
}

static inline void DeInitController(void* data) {
  curl_global_cleanup();
}

static inline bool GetSpecField(const Resource* res, const char* field, char** result, size_t* result_len) {
  ASSERT(res);
  ASSERT(res->spec.doc);

  json_t* source = json_object_get(res->spec.doc, field);
  if (!source || !json_is_string(source))
    return false;

  Expander expander;
  return ExpandStr(&expander, json_string_value(source), result, result_len);
}

static inline bool GetOptionalSpecField(const Resource* res, const char* field, char** result, size_t* result_len) {
  ASSERT(res);
  ASSERT(res->spec.doc);

  json_t* source = json_object_get(res->spec.doc, field);
  if (!source) {
    *result = NULL;
    *result_len = 0;
    return true;
  }

  if (!json_is_string(source))
    return false;

  Expander expander;
  return ExpandStr(&expander, json_string_value(source), result, result_len);
}

static inline bool IsHexSha256(const char* value, const size_t value_len) {
  if (!value || value_len != crypto_hash_sha256_BYTES * 2)
    return false;

  for (size_t i = 0; i < value_len; i++) {
    if (!isxdigit((unsigned char)value[i]))
      return false;
  }

  return true;
}

static inline bool MkdirRecursive(const char* path, const mode_t mode) {
  if (!path || path[0] == '\0')
    return false;

  char buf[PATH_MAX];
  const size_t len = strlen(path);
  if (len >= sizeof(buf))
    return false;

  memcpy(buf, path, len + 1);
  for (char* p = buf + 1; *p; p++) {
    if (*p != '/')
      continue;

    *p = '\0';
    if (mkdir(buf, mode) != 0 && errno != EEXIST)
      return false;
    *p = '/';
  }

  if (mkdir(buf, mode) != 0 && errno != EEXIST)
    return false;

  return true;
}

static inline bool EnsureParentDirExists(const char* path) {
  char buf[PATH_MAX];
  const size_t len = strlen(path);
  if (len >= sizeof(buf))
    return false;

  memcpy(buf, path, len + 1);
  char* slash = strrchr(buf, '/');
  if (!slash || slash == buf)
    return true;

  *slash = '\0';
  return MkdirRecursive(buf, 0777);
}

static inline bool Sha256File(const char* path, char* out) {
  FILE* in = fopen(path, "rb");
  if (!in)
    return false;

  crypto_hash_sha256_state state;
  crypto_hash_sha256_init(&state);

  unsigned char buf[8192];
  size_t n = 0;
  while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
    crypto_hash_sha256_update(&state, buf, n);

  const bool had_error = ferror(in) != 0;
  fclose(in);
  if (had_error)
    return false;

  unsigned char digest[crypto_hash_sha256_BYTES];
  crypto_hash_sha256_final(&state, digest);

  static const char kHex[] = "0123456789abcdef";
  for (size_t i = 0; i < sizeof(digest); i++) {
    out[i * 2] = kHex[digest[i] >> 4];
    out[i * 2 + 1] = kHex[digest[i] & 0x0F];
  }
  out[sizeof(digest) * 2] = '\0';
  return true;
}

static size_t WriteToFile(void* contents, const size_t size, const size_t nmemb, void* userp) {
  return fwrite(contents, size, nmemb, (FILE*)userp);
}

static inline bool DownloadToFile(const char* url, const char* dest) {
  CURL* curl = curl_easy_init();
  if (!curl)
    return false;

  FILE* out = fopen(dest, "wb");
  if (!out) {
    LOG_ERROR("failed to open `%s` for writing: %s", dest, strerror(errno));
    curl_easy_cleanup(curl);
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToFile);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);  // treat HTTP >=400 responses as failures
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "hypha/" HYPHA_VERSION);

  const CURLcode res = curl_easy_perform(curl);
  fclose(out);

  bool success = res == CURLE_OK;
  if (!success)
    LOG_ERROR("failed to download `%s`: %s", url, curl_easy_strerror(res));

  curl_easy_cleanup(curl);
  return success;
}

thread_local DownloadSpec download_spec;

static const char kUrlField[] = "url";
static const char kDestinationField[] = "destination";
static const char kSha256Field[] = "sha256";

DEFINE_CONTROLLER_OBSERVE_FN(Download) {
  Resource* observed = ctx->observed;
  ASSERT(observed);

  if (!observed->spec.doc)
    return kStatusOk;

  if (!GetSpecField(observed, kUrlField, &download_spec.url, &download_spec.url_len)) {
    LOG_ERROR("failed to get `%s` field", kUrlField);
    return kStatusInternalError;
  }

  if (!GetSpecField(observed, kDestinationField, &download_spec.destination, &download_spec.destination_len)) {
    LOG_ERROR("failed to get `%s` field", kDestinationField);
    return kStatusInternalError;
  }

  if (!GetOptionalSpecField(observed, kSha256Field, &download_spec.sha256, &download_spec.sha256_len)) {
    LOG_ERROR("failed to get `%s` field", kSha256Field);
    return kStatusInternalError;
  }

  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Download) {
  ValidationLog* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  if (!download_spec.url || download_spec.url_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` download_spec field", kUrlField);
    return false;
  }

  if (!download_spec.destination || download_spec.destination_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` download_spec field", kDestinationField);
    return false;
  }

  if (download_spec.sha256 && download_spec.sha256_len > 0 &&
      !IsHexSha256(download_spec.sha256, download_spec.sha256_len)) {
    NewFailedValidationResult(log, desired, "`%s` must be a %d-character hex-encoded SHA-256 digest", kSha256Field,
                              (int)(crypto_hash_sha256_BYTES * 2));
    return false;
  }

  NewPassedValidationResult(log, desired, "Spec is valid");
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(Download) {
  Plan* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  struct stat dest_stat;
  if (stat(download_spec.destination, &dest_stat) != 0) {
    PlannedAction* action =
        NewCreatePlannedAction(log, desired, "destination `%s` does not exist", download_spec.destination);
    ASSERT(action);
    return kCreateAction;
  }

  if (!download_spec.sha256 || download_spec.sha256_len == 0) {
    // No checksum given -- idempotency is presence-based only, same as Archive/Repository.
    PlannedAction* action =
        NewNoPlannedAction(log, desired, "destination `%s` already exists", download_spec.destination);
    ASSERT(action);
    return kNoAction;
  }

  char actual[crypto_hash_sha256_BYTES * 2 + 1];
  if (!Sha256File(download_spec.destination, actual)) {
    PlannedAction* action =
        NewFailedPlannedAction(log, desired, "failed to hash existing `%s`", download_spec.destination);
    ASSERT(action);
    return kFailedAction;
  }

  if (strncmp(actual, download_spec.sha256, sizeof(actual)) == 0) {
    PlannedAction* action = NewNoPlannedAction(log, desired, "destination `%s` already has the expected checksum",
                                               download_spec.destination);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action = NewUpdatePlannedAction(log, desired, "destination `%s` has checksum `%s`, expected `%s`",
                                                 download_spec.destination, actual, download_spec.sha256);
  ASSERT(action);
  return kUpdateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Download) {
  Resource* desired = ctx->desired;
  ASSERT(desired);

  if (!EnsureParentDirExists(download_spec.destination)) {
    LOG_ERROR("failed to create parent directory for `%s`: %s", download_spec.destination, strerror(errno));
    return kStatusInternalError;
  }

  char tmp_path[PATH_MAX];
  const int n = snprintf(tmp_path, sizeof(tmp_path), "%s.hypha-download-tmp.%d", download_spec.destination, getpid());
  if (n < 0 || (size_t)n >= sizeof(tmp_path)) {
    LOG_ERROR("destination path `%s` is too long", download_spec.destination);
    return kStatusInternalError;
  }

  if (!DownloadToFile(download_spec.url, tmp_path)) {
    remove(tmp_path);
    return kStatusInternalError;
  }

  if (download_spec.sha256 && download_spec.sha256_len > 0) {
    char actual[crypto_hash_sha256_BYTES * 2 + 1];
    if (!Sha256File(tmp_path, actual)) {
      LOG_ERROR("failed to hash downloaded `%s`", tmp_path);
      remove(tmp_path);
      return kStatusInternalError;
    }

    if (strncmp(actual, download_spec.sha256, sizeof(actual)) != 0) {
      LOG_ERROR("`%s` has checksum `%s`, expected `%s`", download_spec.url, actual, download_spec.sha256);
      remove(tmp_path);
      return kStatusInternalError;
    }
  }

  if (rename(tmp_path, download_spec.destination) != 0) {
    LOG_ERROR("failed to move `%s` to `%s`: %s", tmp_path, download_spec.destination, strerror(errno));
    remove(tmp_path);
    return kStatusInternalError;
  }

  AppliedAction* action =
      ctx->action == kUpdateAction
          ? NewUpdateAction(ctx->log, desired, "`%s` downloaded to `%s`", download_spec.url, download_spec.destination)
          : NewCreateAction(ctx->log, desired, "`%s` downloaded to `%s`", download_spec.url, download_spec.destination);
  ASSERT(action);
  return kStatusOk;
}

// Status and Diff ask the same question here -- "does destination exist and, if a checksum
// was given, does it match" -- so they share one implementation. `dlog` is NULL from Status
// (plain pass/fail, logs failures normally) and non-NULL from Diff (records the same
// finding, success or failure, as a Delta).
static inline ControllerStatus CheckDownloadUpToDate(const Resource* observed, DeltaLog* dlog) {
  ASSERT(observed);

  struct stat dest_stat;
  if (stat(download_spec.destination, &dest_stat) != 0) {
    if (dlog) {
      NewNoDelta(dlog, "destination `%s` does not exist", download_spec.destination);
    } else {
      LOG_ERROR("destination `%s` does not exist", download_spec.destination);
    }
    return kStatusInternalError;
  }

  if (download_spec.sha256 && download_spec.sha256_len > 0) {
    char actual[crypto_hash_sha256_BYTES * 2 + 1];
    if (!Sha256File(download_spec.destination, actual)) {
      if (dlog) {
        NewNoDelta(dlog, "failed to hash `%s`", download_spec.destination);
      } else {
        LOG_ERROR("failed to hash `%s`", download_spec.destination);
      }
      return kStatusInternalError;
    }

    if (strncmp(actual, download_spec.sha256, sizeof(actual)) != 0) {
      if (dlog) {
        NewNoDelta(dlog, "`%s` has checksum `%s`, expected `%s`", download_spec.destination, actual,
                   download_spec.sha256);
      } else {
        LOG_ERROR("`%s` has checksum `%s`, expected `%s`", download_spec.destination, actual, download_spec.sha256);
      }
      return kStatusInternalError;
    }
  }

  if (dlog)
    NewNoDelta(dlog, "destination `%s` is up to date", download_spec.destination);

  return kStatusOk;
}

static inline ControllerStatus DownloadCheckStatus(StatusContext* ctx, void* data) {
  return CheckDownloadUpToDate(ctx->current, NULL);
}

static inline ControllerStatus DownloadCheckDiff(DiffContext* ctx, void* data) {
  return CheckDownloadUpToDate(ctx->observed, ctx->log);
}

static const ControllerConfig kDownloadControllerConfig = {
    .init = InitController,
    .deinit = DeInitController,
    .observe = DownloadObserve,
    .plan = DownloadPlan,
    .apply = DownloadApply,
    .validate = DownloadValidate,
    .diff = DownloadCheckDiff,
    .status = DownloadCheckStatus,
    .rollback = NULL,
    .normalize = NULL,
    .destroy = NULL,
};

static ResourceKind kDownloadKind = kInvalidResourceKind;

ResourceKind GetDownloadResourceKind() {
  return kDownloadKind;
}

Controller* NewDownloadController() {
  kDownloadKind = NewResourceKind(kDownloadControllerKindName);
  if (kDownloadKind == kInvalidResourceKind)
    return NULL;

  const char* aliases[2] = {
      "download",
      "downloads",
  };
  return NewController(kDownloadKind, kDownloadControllerConfig, aliases, 2, NULL, NULL);
}
