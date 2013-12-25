/*
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"
#include "unpack_interface.h"
#include "mounts_reader.h"
#include "parse_path.h"
#include "mounts_interface.h"
#include "image_engine.h"
#include "enum_strings.h"

static char block[512];
static struct ParsePathObserver s_path_observer;

#define WRITE_DATA_INTO_FS_IN_MEMORY( mounts_p, wrote_p, err_p, out_fd, data, datasize )	\
    *wrote_p = (mounts_p)->write(mounts_p,out_fd, block, datasize );		\
    if ( *wrote_p < datasize ){						\
	*err_p=1;							\
	ZRT_LOG(L_ERROR, "block write error, wrote %d instead %d bytes", \
		*wrote_p, datasize );					\
    }


//////////////////////////// parse path callback implementation //////////////////////////////

/*directories handler, it's responsible to create existing directories at path*/
static void callback_parse(struct ParsePathObserver* this_p, const char *path, int length){
    /*do not handle short paths*/
    if ( length < 2 ) return;
    /*do not create dir if already cached*/
    create_dir_and_cache_name(path, length);
}

//////////////////////////// unpack observer implementation //////////////////////////////

/*unpack observer 1st parameter : main unpack interface that gives access to observer, mounts and mounted fs*/
static int extract_entry( struct UnpackInterface* unpacker, 
			  TypeFlag type, const char* name, int entry_size ){
    /*parse path and create directories recursively*/
    ZRT_LOG( L_INFO, "type=%s, name=%s, entry_size=%d", 
	     STR_ARCH_ENTRY_TYPE(type), name, entry_size );

    /*setup path parser observer
     *observers callback will be called for every paursed subdir extracted from full path*/
    s_path_observer.callback_parse = callback_parse;
    s_path_observer.anyobj = unpacker->observer->mounts;

    /*run path parser*/
    int parsed_dir_count = parse_path( &s_path_observer, name );
    ZRT_LOG(L_INFO, "parsed_dir_count=%d", parsed_dir_count );

    if ( type == ETypeDir ){
	create_dir_and_cache_name(name, strlen(name));
    }
    else{
	/*Create new file, truncate it if exist*/
	int out_fd = unpacker->observer->mounts->open( unpacker->observer->mounts,
						       name, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if (out_fd < 0) {
	    ZRT_LOG( L_ERROR, "create new file error, name=%s", name );
	    return -1;
	}

	ZRT_LOG(L_SHORT, "save %7d B : %s", entry_size, name);
	int should_write = entry_size;
	int write_err = 0;
        /*read file by blocks*/
        while (entry_size > 0) {
            int len = (*unpacker->mounts_reader->read)( unpacker->mounts_reader,
							block, sizeof(block) );
            if (len != sizeof(block)) {
		ZRT_LOG(L_ERROR, "read error. saving failed=%s", name);
                return -1;
            }
	    int wrote;
            if (entry_size > sizeof(block)) {
		WRITE_DATA_INTO_FS_IN_MEMORY( unpacker->observer->mounts,
					      &wrote, &write_err, out_fd, block, sizeof(block) );
            } else {
		WRITE_DATA_INTO_FS_IN_MEMORY( unpacker->observer->mounts,
					      &wrote, &write_err, out_fd, block, entry_size );
            }
	    if ( write_err ){
		return -1;
	    }
            entry_size -= sizeof(block);
        }
	unpacker->observer->mounts->close(unpacker->observer->mounts,
					  out_fd);
    }
    return 0;
}

static struct UnpackObserver s_unpack_observer = {
        extract_entry,
        NULL
};

static int deploy_image( const char* mount_path, struct UnpackInterface* unpacker ){
    assert(unpacker);
    ZRT_LOG(L_SHORT, "mount_path=%s", mount_path );
    create_dir_and_cache_name(mount_path, strlen(mount_path));
    return unpacker->unpack( unpacker, mount_path );
}

//////////////////////////// image engine implementation //////////////////////////////

struct ImageInterface* alloc_image_loader( struct MountsPublicInterface* mounts ){
    struct ImageInterface* image_engine = malloc( sizeof(struct ImageInterface) );
    image_engine->deploy_image = deploy_image; /*setup function pointer*/
    image_engine->mounts = mounts; /*set filesystem to extract files*/
    image_engine->observer_implementation = &s_unpack_observer; /*set observer unpack*/
    image_engine->observer_implementation->mounts = mounts;
    return image_engine;
}

void free_image_loader( struct ImageInterface* image_engine ){
    free( image_engine );
}



