/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
  char filename[100];
  char mode[8];
  char owner_numeric[8];
  char group_numeric[8];
  char size[12];
  char last_modified[12];
  char checksum[8];
  char link;
  char linked_filename[100];
  char ustar[6];
  char version[2];
  char owner[32];
  char group[32];
  char major[8];
  char minor[8];
  char filename_prefix[155];
} TAR_HEADER;


int simple_tar_extract_to(const char *tarfile, const char *dst) {
  char block[512];
  char filename[256];
  char dst_filename[MAXPATHLEN];
  TAR_HEADER *header = (TAR_HEADER*)block;
  int file_len;
  int len;
  FILE *file;
  FILE *out;
  int count;

  count = 0;
  file = fopen(tarfile, "rb");
  if (!file) return -1;
  for (;;) {
    len = fread(block, 1, sizeof(block), file);
    if (!len) break;
    if (len != sizeof(block)) {
      fclose(file);
      return -1;
    }

    memset(filename, 0, sizeof(filename));
    if (memcmp(header->ustar, "ustar", 5) == 0) {
      memcpy(filename, header->filename_prefix,
             sizeof(header->filename_prefix));
    }
    strcat(filename, header->filename);

    if (!strlen(filename)) break;

    /* Check that dst + "/" + filename + '\0' fits in MAXPATHLEN. */
    if (strlen(dst) + strlen(filename) + 2 > MAXPATHLEN) {
      return -1;
    }
    strcpy(dst_filename, dst);
    strcat(dst_filename, "/");
    strcat(dst_filename, filename);

    if (dst_filename[strlen(dst_filename) - 1] == '/') {
      mkdir(dst_filename, 0777);
      continue;
    }

    if (sscanf(header->size, "%o", &file_len) != 1) {
      fclose(file);
      return -1;
    }
    out = fopen(dst_filename, "wb");
    if (!out) {
      fclose(file);
      return -1;
    }
    while (file_len > 0) {
      len = fread(block, 1, sizeof(block), file);
      if (len != sizeof(block)) {
        return -1;
      }
      if (file_len > sizeof(block)) {
        fwrite(block, 1, sizeof(block), out);
      } else {
        fwrite(block, 1, file_len, out);
      }
      file_len -= sizeof(block);
    }
    fclose(out);

    ++count;
  }
  fclose(file);
  return count;
}


int simple_tar_extract(const char *tarfile) {
  char cwd[MAXPATHLEN];
  char *cwdp;

  cwdp = getcwd(cwd, sizeof(cwd));
  if (!cwdp) {
    return -1;
  }
  return simple_tar_extract_to(tarfile, cwd);
}
