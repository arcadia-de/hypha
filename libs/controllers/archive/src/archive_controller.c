#include "hypha/archive_controller.h"

#include <archive.h>
#include <archive_entry.h>
#include <errno.h>
#include <jansson.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/archive_spec.h"
#include "hypha/controller_status.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/reason.h"
#include "hypha/resource_kind.h"
#include "hypha/validation_log.h"

static const uint32_t kDefaultBufferSize = 16384;

static inline bool GetSpecField(const Resource* res, const char* field, char** result, size_t* result_len) {
  ASSERT(res);
  ASSERT(res->spec.doc);

  json_t* source = json_object_get(res->spec.doc, field);
  if (!source || !json_is_string(source))
    return false;

  Expander expander;
  return ExpandStr(&expander, json_string_value(source), result, result_len);
}

// Recursively creates `path` (and any missing parents), mirroring `mkdir -p`. Addresses the
// long-standing TODO in Extract() below: libarchive will happily write entries under a
// destination directory that doesn't exist yet, but only once the root of that tree is there.
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

static inline bool Extract(const char* filename, const char* out_dir, const uint32_t buffer_size) {
  if (!MkdirRecursive(out_dir, 0777)) {
    LOG_ERROR("failed to create destination directory `%s`: %s", out_dir, strerror(errno));
    return false;
  }

  struct archive* a = archive_read_new();
  struct archive* ext = archive_write_disk_new();
  struct archive_entry* entry = NULL;
  la_ssize_t r = 0;
  bool success = false;

  int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
  archive_write_disk_set_options(ext, flags);
  archive_write_disk_set_standard_lookup(ext);

  archive_read_support_format_tar(a);
  archive_read_support_format_zip(a);
  archive_read_support_filter_gzip(a);
  archive_read_support_filter_bzip2(a);
  archive_read_support_filter_xz(a);

  if (archive_read_open_filename(a, filename, buffer_size) != ARCHIVE_OK) {
    LOG_ERROR("failed to open archive `%s`: %s", filename, archive_error_string(a));
    goto finished;
  }

  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    {
      const char* current_path = archive_entry_pathname(entry);
      char new_path[PATH_MAX];
      snprintf(new_path, sizeof(new_path), "%s/%s", out_dir, current_path);
      archive_entry_set_pathname(entry, new_path);
    }

    r = archive_write_header(ext, entry);
    if (r < ARCHIVE_OK) {
      LOG_WARN("failed to write header for `%s`: %s", archive_entry_pathname(entry), archive_error_string(ext));
    } else if (archive_entry_size(entry) > 0) {
      const void* buff = NULL;
      size_t size = 0;
      la_int64_t offset = 0;

      while (1) {
        r = archive_read_data_block(a, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
          break;

        if (r < ARCHIVE_OK) {
          LOG_ERROR("failed to read data block from `%s`: %s", filename, archive_error_string(a));
          goto finished;
        }

        r = archive_write_data_block(ext, buff, size, offset);
        if (r < ARCHIVE_OK) {
          LOG_ERROR("failed to write data block to `%s`: %s", out_dir, archive_error_string(ext));
          goto finished;
        }
      }
    }

    archive_write_finish_entry(ext);
  }

  success = true;
finished:
  archive_read_free(a);
  archive_write_free(ext);
  return success;
}

thread_local ArchiveSpec archive_spec;

static const char kSourceField[] = "source";
static const char kDestinationField[] = "destination";

DEFINE_CONTROLLER_OBSERVE_FN(Archive) {
  Resource* observed = ctx->observed;
  ASSERT(observed);

  if (!observed->spec.doc)
    return kStatusOk;

  if (!GetSpecField(observed, kSourceField, &archive_spec.source, &archive_spec.source_len)) {
    LOG_ERROR("failed to get `%s` field", kSourceField);
    return kStatusInternalError;
  }

  if (!GetSpecField(observed, kDestinationField, &archive_spec.destination, &archive_spec.destination_len)) {
    LOG_ERROR("failed to get `%s` field", kDestinationField);
    return kStatusInternalError;
  }

  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Archive) {
  ValidationLog* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  if (!archive_spec.source || archive_spec.source_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` archive_spec field", kSourceField);
    return false;
  }

  if (!archive_spec.destination || archive_spec.destination_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` archive_spec field", kDestinationField);
    return false;
  }

  struct stat source_stat;
  if (stat(archive_spec.source, &source_stat) != 0) {
    NewFailedValidationResult(log, desired, "Archive `%s` does not exist", archive_spec.source);
    return false;
  }

  NewPassedValidationResult(log, desired, "Spec is valid");
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(Archive) {
  Plan* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  struct stat source_stat;
  if (stat(archive_spec.source, &source_stat) != 0) {
    PlannedAction* action = NewNoPlannedAction(log, desired, "Source `%s` does not exist", archive_spec.source);
    ASSERT(action);
    return kNoAction;
  }

  struct stat dest_stat;
  if (stat(archive_spec.destination, &dest_stat) == 0) {
    if (S_ISDIR(dest_stat.st_mode)) {
      PlannedAction* action =
          NewNoPlannedAction(log, desired, "Destination `%s` already exists", archive_spec.destination);
      // TODO(@s0cks): no way yet to detect a stale extraction (e.g. archive changed since
      // last apply) -- this only ever short-circuits once the destination exists at all.
      ASSERT(action);
      return kNoAction;
    }

    PlannedAction* action =
        NewNoPlannedAction(log, desired, "Destination `%s` exists but is not a directory", archive_spec.destination);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action =
      NewCreatePlannedAction(log, desired, "Destination `%s` doesn't exist", archive_spec.destination);
  ASSERT(action);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Archive) {
  Resource* desired = ctx->desired;
  ASSERT(desired);

  if (!Extract(archive_spec.source, archive_spec.destination, kDefaultBufferSize)) {
    LOG_ERROR("failed to extract `%s` into `%s`", archive_spec.source, archive_spec.destination);
    return kStatusInternalError;
  }

  AppliedAction* action = NewCreateAction(ctx->log, desired, "Archive `%s` extracted to `%s`", archive_spec.source,
                                          archive_spec.destination);
  ASSERT(action);
  return kStatusOk;
}

DEFINE_CONTROLLER_STATUS_FN(Archive) {
  const Resource* current = ctx->current;
  ASSERT(current);

  struct stat dest_stat;
  if (stat(archive_spec.destination, &dest_stat) != 0 || !S_ISDIR(dest_stat.st_mode)) {
    LOG_ERROR("Destination `%s` does not exist", archive_spec.destination);
    return kStatusInternalError;
  }

  return kStatusOk;
}

static const ControllerConfig kArchiveControllerConfig = {
    .init = NULL,
    .deinit = NULL,
    .observe = ArchiveObserve,
    .plan = ArchivePlan,
    .apply = ArchiveApply,
    .validate = ArchiveValidate,
    .diff = NULL,
    .status = ArchiveStatus,
    .rollback = NULL,
    .normalize = NULL,
    .destroy = NULL,
};

static ResourceKind kArchiveKind = kInvalidResourceKind;

ResourceKind GetArchiveResourceKind() {
  return kArchiveKind;
}

Controller* NewArchiveController() {
  kArchiveKind = NewResourceKind(kArchiveControllerKindName);
  if (kArchiveKind == kInvalidResourceKind)
    return NULL;

  const char* aliases[2] = {
      "archive",
      "archives",
  };
  return NewController(kArchiveKind, kArchiveControllerConfig, aliases, 2, NULL, NULL);
}
