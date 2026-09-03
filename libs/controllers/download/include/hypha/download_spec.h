#ifndef HYPHA_DOWNLOAD_SPEC_H
#define HYPHA_DOWNLOAD_SPEC_H

#include <stddef.h>

typedef struct {
  char* url;
  size_t url_len;

  char* destination;
  size_t destination_len;

  // Optional. Lowercase hex-encoded SHA-256 digest (64 chars) the downloaded file is
  // expected to have. When absent, idempotency is presence-based only (like Archive and
  // Repository): a destination that already exists is left alone. When present, a
  // destination whose contents don't match is treated as needing an Update, not just a
  // Create -- the same way Template distinguishes "doesn't exist" from "exists with the
  // wrong content."
  char* sha256;
  size_t sha256_len;
} DownloadSpec;

#endif  // HYPHA_DOWNLOAD_SPEC_H
