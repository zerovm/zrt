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

#include "mounts_interface.h"
#include "nacl_simple_tar.h"
#include "nacllog.h"

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


int simple_tar_extract(const char *tar_image, const char* extract_path, struct MountsInterface* fs) {
    char block[512];
    char filename[256];
    char dst_filename[MAXPATHLEN];
    TAR_HEADER *header = (TAR_HEADER*)block;
    int file_len;
    int len;
    int image_fd;
    int out_fd;
    int count;

    count = 0;
    zrt_log( "open image=%s", tar_image );
    image_fd = fs->open(tar_image, O_RDONLY );
    zrt_log( "image_fd=%d", image_fd );
    if (image_fd < 0) return -1;
    for (;;) {
        zrt_log( "image_fd=%d", image_fd );
        len = fs->read( image_fd, block, sizeof(block) );
        if (!len) break;
        if (len != sizeof(block)) {
            fs->close(image_fd);
            return -1;
        }

        memset(filename, 0, sizeof(filename));
        if (memcmp(header->ustar, "ustar", 5) == 0) {
            memcpy(filename, header->filename_prefix,
                    sizeof(header->filename_prefix));
        }
        strcat(filename, header->filename);

        if (!strlen(filename)) break;

        /* Check that extract_path + "/" + filename + '\0' fits in MAXPATHLEN. */
        if (strlen(extract_path) + strlen(filename) + 2 > MAXPATHLEN) {
            return -1;
        }
        strcpy(dst_filename, extract_path);
        strcat(dst_filename, "/");
        strcat(dst_filename, filename);

        if (dst_filename[strlen(dst_filename) - 1] == '/') {
            zrt_log( "create dir =%s", dst_filename );
            int ret = fs->mkdir(dst_filename, 0777);
            zrt_log( "dir ret=%d", ret );
            continue;
        }

        if (sscanf(header->size, "%o", &file_len) != 1) {
            fs->close(image_fd);
            return -1;
        }
        zrt_log( "create new file =%s", dst_filename );
        out_fd = fs->open(dst_filename, O_WRONLY | O_CREAT, S_IRWXU);
        if (out_fd < 0) {
            zrt_log( "create error file =%s", dst_filename );
            return -1;
        }
        while (file_len > 0) {
            len = fs->read( image_fd, block, sizeof(block) );
            if (len != sizeof(block)) {
                return -1;
            }
            if (file_len > sizeof(block)) {
                fs->write(out_fd, block, sizeof(block) );
            } else {
                fs->write(out_fd, block, file_len );
            }
            file_len -= sizeof(block);
        }
        fs->close(out_fd);

        ++count;
    }
    fs->close(image_fd);
    zrt_log( "created %d files", count );
    return count;
}


