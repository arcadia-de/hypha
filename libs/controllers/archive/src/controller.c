#include <archive.h>
#include <archive_entry.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "archive_controller.h"
#include "hypha.h"
#include "hypha/log.h"

static inline bool Extract(const char* filename, const char* out_dir, const uint32_t buffer_size) {
  // TODO(@s0cks): need to create the initial root dir
  struct archive* a = archive_read_new();
  struct archive* ext = archive_write_disk_new();
  struct archive_entry* entry = NULL;
  la_ssize_t r = 0;

  int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
  archive_write_disk_set_options(ext, flags);
  archive_write_disk_set_standard_lookup(ext);

  archive_read_support_format_tar(a);
  archive_read_support_filter_gzip(a);

  if (archive_read_open_filename(a, filename, buffer_size) != ARCHIVE_OK)
    return false;

  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    {
      const char* current_path = archive_entry_pathname(entry);
      char new_path[PATH_MAX];
      snprintf(new_path, sizeof(new_path), "%s/%s", out_dir, current_path);
      archive_entry_set_pathname(entry, new_path);
    }

    r = archive_write_header(ext, entry);
    if (r < ARCHIVE_OK) {
      //????
    } else if (archive_entry_size(entry) > 0) {
      const void* buff = NULL;
      size_t size = 0;
      la_int64_t offset = 0;

      while (1) {
        r = archive_read_data_block(a, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
          break;

        if (r < ARCHIVE_OK)
          return false;

        r = archive_write_data_block(ext, buff, size, offset);
        if (r < ARCHIVE_OK)
          return false;
      }
    }

    archive_write_finish_entry(ext);
  }

  archive_read_free(a);
  archive_write_free(ext);
  return true;
}

DEFINE_CONTROLLER_OBSERVE_FN(Archive) {
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(Archive) {
  return kNoAction;
}

DEFINE_CONTROLLER_APPLY_FN(Archive) {
  return kStatusOk;
}

static const ControllerConfig kArchiveControllerConfig = {
    .observe = ArchiveObserve,
    .plan = ArchivePlan,
    .apply = ArchiveApply,
};
DEFINE_NEW_CONTROLLER(Archive, ARCHIVE_CONTROLLER_KIND);
