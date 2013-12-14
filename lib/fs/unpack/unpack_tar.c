/*
 *
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Copyright 2008, Google Inc.
 * All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Google Inc. nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

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
#include "mounts_reader.h"
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
    while( (len=unpack_if->mounts_reader->read( unpack_if->mounts_reader, 
						block, 
						sizeof(block)) ) != 0 ) {
	if (len != sizeof(block)) {
            /*every file size should be aligned to 512bytes in generic case*/
            ZRT_LOG(L_ERROR, P_TEXT, "file block not aligned" );
            return -EUnpackStateNotImplemented;
        }

	//get file size
	if (sscanf(header->size, "%o", &file_len) != 1) {
	    ZRT_LOG(L_ERROR, P_TEXT, "unpack_tar: can't read item size");
	    break;
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
    ZRT_LOG( L_SHORT, "unpacked items count: %d", count );
    return count;

}


struct UnpackInterface* alloc_unpacker_tar( struct MountsReader* mounts_reader, struct UnpackObserver* observer ){
    struct UnpackInterface* unpacker_tar = malloc( sizeof(struct UnpackInterface) );
    unpacker_tar->unpack = unpack_tar;
    unpacker_tar->mounts_reader = mounts_reader;
    unpacker_tar->observer = observer;
    return unpacker_tar;
}


void free_unpacker_tar( struct UnpackInterface* unpacker_tar ){
    free( unpacker_tar );
}

