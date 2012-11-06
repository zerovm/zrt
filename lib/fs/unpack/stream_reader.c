/*
 * stream_reader.c
 *
 *  Created on: 27.09.2012
 *      Author: yaroslav
 */


#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "zrtlog.h"
#include "stream_reader.h"
#include "mounts_interface.h"


ssize_t stream_read(struct StreamReader* reader, void *buf, size_t nbyte){
    return (*reader->mounts_interface->read)( reader->fd, buf, nbyte );
}


struct StreamReader* alloc_stream_reader( struct MountsInterface* mounts_interface, const char* channel_name ){
    assert( mounts_interface );
    int fd = mounts_interface->open( channel_name, O_RDONLY, 0 );
    if ( fd < 0 ){
        ZRT_LOG( L_ERROR, "failed to open image channel %s", channel_name );
        return NULL;
    }

    struct StreamReader* stream_reader = malloc( sizeof(struct StreamReader) );
    stream_reader->fd = fd;
    stream_reader->read = stream_read;
    stream_reader->mounts_interface = mounts_interface;
    return stream_reader;
}

void free_stream_reader( struct StreamReader* stream_reader ){
    assert( stream_reader );
    ZRT_LOG( L_SHORT, "close channel fd=%d", stream_reader->fd );
    stream_reader->mounts_interface->close( stream_reader->fd );
    free(stream_reader);
}


