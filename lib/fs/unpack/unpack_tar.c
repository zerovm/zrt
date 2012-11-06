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
#include <errno.h>

#include "zrtlog.h"
#include "stream_reader.h"
#include "unpack_interface.h"
#include "mounts_interface.h"
#include "unpack_tar.h"


#define DIRTYPE  '5'            /* directory */

typedef struct {
    char filename[100];
    char mode[8];
    char owner_numeric[8];
    char group_numeric[8];
    char size[12];
    char last_modified[12];
    char checksum[8];
    char typeflag;
    char linked_filename[100];
    char ustar[6];
    char version[2];
    char owner[32];
    char group[32];
    char major[8];
    char minor[8];
    char filename_prefix[155];
} TAR_HEADER;


static int unpack_tar( struct UnpackInterface* unpack_if, const char* mount_path ){
    ZRT_LOG(L_INFO, "%s", mount_path);
    char block[512];
    char filename[256];
    char dst_filename[MAXPATHLEN];
    TAR_HEADER *header = (TAR_HEADER*)block;
    int file_len;
    int len;
    int count;

    count = 0;
    for (;;) {
        len =  unpack_if->stream_reader->read( unpack_if->stream_reader, block, sizeof(block));
        if (!len) break;
        if (len != sizeof(block)) {
            /*every file size should be aligned to 512bytes in generic case*/
            ZRT_LOG(L_ERROR, "ret=%s", "EUnpackStateNotImplemented");
            return EUnpackStateNotImplemented;
        }

        memset(filename, 0, sizeof(filename));
        if (memcmp(header->ustar, "ustar", 5) == 0) {
            memcpy(filename, header->filename_prefix,
                    sizeof(header->filename_prefix));
        }
        strcat(filename, header->filename);

        if (!strlen(filename)) break;

        /* Check that mount_path + "/" + filename + '\0' fits in MAXPATHLEN. */
        int pathlen=0;
        if ((pathlen=strlen(mount_path) + strlen(filename) + 2) > MAXPATHLEN) {
            ZRT_LOG(L_ERROR, "to big path readed from archive. len=%d", pathlen);
            return EUnpackToBigPath;
        }

        strcpy(dst_filename, mount_path);
        if ( !(strlen(mount_path) == 1 && mount_path[0] == '/') ){
            strcat(dst_filename, "/");
        }
        strcat(dst_filename, filename);

        if (sscanf(header->size, "%o", &file_len) != 1) {
	    ZRT_LOG(L_ERROR, "ret=%s", "unknown");
            return -1;
        }

        TypeFlag type = ETypeFile;
        if ( header->typeflag == DIRTYPE ){
            type = ETypeDir;
        }
        unpack_if->observer->extract_entry( unpack_if, type, dst_filename, file_len );
        ++count;
    }
    ZRT_LOG( L_SHORT, "created %d files", count );
    return count;

}


struct UnpackInterface* alloc_unpacker_tar( struct StreamReader* stream_reader, struct UnpackObserver* observer ){
    struct UnpackInterface* unpacker_tar = malloc( sizeof(struct UnpackInterface) );
    unpacker_tar->unpack = unpack_tar;
    unpacker_tar->stream_reader = stream_reader;
    unpacker_tar->observer = observer;
    return unpacker_tar;
}


void free_unpacker_tar( struct UnpackInterface* unpacker_tar ){
    free( unpacker_tar );
}

