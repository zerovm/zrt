/*
 * image_engine.c
 *
 *  Created on: 25.09.2012
 *      Author: yaroslav
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
#include "stream_reader.h"
#include "unpack_interface.h"
#include "parse_path.h"
#include "mounts_interface.h"
#include "image_engine.h"

static char block[512];

//////////////////////////// parse path callback implementation //////////////////////////////

static int callback_parse(struct ParsePathObserver* this_p, const char *path, int length){
    if ( length < 2 ) return 0;
    /*received callback, path is a directory path, it is guaranteed that nesting level is increasing
     * for every next callback, so we can just ceate directories*/
    ZRT_LOG( L_INFO, "path=%s", path );
    char* dir_path = calloc(1, length+1); /*alloc string, should be freed after use*/
    strncpy( dir_path, path, length );
    struct MountsInterface* mounts = (struct MountsInterface*)this_p->anyobj;
    int ret = mounts->mkdir( dir_path, S_IRWXU );
    ZRT_LOG( L_INFO, "mkdir ret=%d, errno=%d", ret, errno );
    free(dir_path); /*free string buffer*/
    return ret;
}

//////////////////////////// unpack observer implementation //////////////////////////////

/*unpack observer 1st parameter : main unpack interface that gives access to observer, stream and mounted fs*/
static int extract_entry( struct UnpackInterface* unpacker, TypeFlag type, const char* name, int entry_size ){
    /*parse path and create directories recursively*/
    ZRT_LOG( L_INFO, "type=%d, name=%s, entry_size=%d", type, name, entry_size );

    /*setup path parser observer
     *observers callback will be called for every paursed subdir extracted from full path*/
    struct ParsePathObserver path_observer;
    path_observer.callback_parse = callback_parse;
    path_observer.anyobj = unpacker->observer->mounts;

    /*run path parser*/
    int parsed_dir_count = parse_path( &path_observer, name );
    ZRT_LOG(L_INFO, "parsed_dir_count=%d", parsed_dir_count );

    int out_fd = unpacker->observer->mounts->open(name, O_WRONLY | O_CREAT, S_IRWXU);
    if (out_fd < 0) {
        ZRT_LOG( L_ERROR, "create new file error, name=%s", name );
        return -1;
    }

    if ( type == ETypeDir ){
        int ret = unpacker->observer->mounts->mkdir( name, 0777 );
        if ( ret == -1 && errno != EEXIST ){
	    ZRT_LOG( L_ERROR, "dir create error, errno=%d", errno );
            return -1; /*dir create error*/
        }
    }
    else{
        /*read file by blocks*/
        while (entry_size > 0) {
            int len = (*unpacker->stream_reader->read)( unpacker->stream_reader,
                    block, sizeof(block) );
            if (len != sizeof(block)) {
                return -1;
            }
            if (entry_size > sizeof(block)) {
                unpacker->observer->mounts->write(out_fd, block, sizeof(block) );
            } else {
                unpacker->observer->mounts->write(out_fd, block, entry_size );
            }
            entry_size -= sizeof(block);
        }
    }
    unpacker->observer->mounts->close(out_fd);
    return 0;
}

static struct UnpackObserver s_unpack_observer = {
        extract_entry,
        NULL
};

static int deploy_image( const char* mount_path, struct UnpackInterface* unpacker ){
    assert(unpacker);
    ZRT_LOG(L_SHORT, "mount_path=%s", mount_path );
    return unpacker->unpack( unpacker, mount_path );
}

//////////////////////////// image engine implementation //////////////////////////////

struct ImageInterface* alloc_image_loader( struct MountsInterface* mounts ){
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



