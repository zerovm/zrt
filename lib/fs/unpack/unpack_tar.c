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
#define USTAR_STR  "ustar"
#define USTAR_LEN  5

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

/*unpacker for "ustar" tar archive*/
static int unpack_tar( struct UnpackInterface* unpack_if, const char* mount_path ){
    ZRT_LOG(L_INFO, "%s", mount_path);
    char block[512];
    char dst_filename[MAXPATHLEN+1];
    TAR_HEADER *header = (TAR_HEADER*)block;
    int file_len;
    int len;
    int count;
    char *backslash;

    /*mount path will const for all files in loop, so add it at first*/
    strcat(dst_filename, mount_path); 

    count = 0;
    while( (len=unpack_if->stream_reader->read( unpack_if->stream_reader, 
						block, 
						sizeof(block)) ) != 0 ) {
	if (len != sizeof(block)) {
            /*every file size should be aligned to 512bytes in generic case*/
            ZRT_LOG(L_ERROR, P_TEXT, "file block not aligned" );
            return -EUnpackStateNotImplemented;
        }

	//get file size
	if (sscanf(header->size, "%o", &file_len) != 1) {
	    ZRT_LOG(L_ERROR, "ret=%s", "unknown");
	    return -1;
	}
	//check filename
	if ( !strlen(header->filename) ) break;
	if ( (strlen(mount_path) > 0 && mount_path[strlen(mount_path)-1] == '/') )
	    backslash = "";
	else
	    backslash = "/";
	    
	//construct full filename
	if ( MAXPATHLEN < snprintf(dst_filename, MAXPATHLEN+1, "%s%s%s",
				   mount_path, backslash, header->filename ) ){
	    ZRT_LOG(L_ERROR, P_TEXT, "To big path readed from archive");
	    return -EUnpackToBigPath;
	}

	TypeFlag type = ETypeFile;
	if ( header->typeflag == DIRTYPE ){
	    type = ETypeDir;
	}
	/* Now item name is retrieved from archive, 
	 * in case if item type is directory we just create it on filesystem,
	 * in case of file it's ready to retrieve data and create it on filesystem */
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

