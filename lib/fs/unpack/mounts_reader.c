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



#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "zrtlog.h"
#include "mounts_reader.h"
#include "mounts_interface.h"

ssize_t mounts_read(struct MountsReader* reader, void *buf, size_t nbyte){
    return reader->buffered_io_reader->read(reader->buffered_io_reader, reader->fd, buf, nbyte);
}


struct MountsReader* alloc_mounts_reader( struct MountsInterface* mounts_interface, const char* channel_name ){
    assert( mounts_interface );
    int fd = mounts_interface->open( channel_name, O_RDONLY, 0 );
    if ( fd < 0 ){
        ZRT_LOG( L_ERROR, "failed to open image channel %s", channel_name );
        return NULL;
    }

    /*seek to channel beginning to support re-read of channel data*/
    int err = mounts_interface->lseek(fd, 0, SEEK_SET);
    if ( err != 0 ){
	ZRT_LOG( L_ERROR, "%s must be seekable channel, err=%d", channel_name, err );
	return NULL;
    }

    struct MountsReader* mounts_reader = malloc( sizeof(struct MountsReader) );
    mounts_reader->buffer = malloc(BUFFER_IO_SIZE);
    mounts_reader->buffered_io_reader = 
	AllocBufferedIORead( mounts_reader->buffer, BUFFER_IO_SIZE, 
			     mounts_interface->read /*override read for buffered io*/ );
    mounts_reader->fd = fd;
    mounts_reader->read = mounts_read;
    mounts_reader->mounts_interface = mounts_interface;
    return mounts_reader;
}

void free_mounts_reader( struct MountsReader* mounts_reader ){
    assert( mounts_reader );
    ZRT_LOG( L_SHORT, "close channel fd=%d", mounts_reader->fd );
    mounts_reader->mounts_interface->close( mounts_reader->fd );
    free(mounts_reader->buffered_io_reader);
    free(mounts_reader->buffer);
    free(mounts_reader);
}


