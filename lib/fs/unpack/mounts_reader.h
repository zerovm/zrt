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

#ifndef MOUNTS_READER_H_
#define MOUNTS_READER_H_

#include <unistd.h> //ssize_t
#include "buffered_io.h"

#define BUFFER_IO_SIZE 1024*64

struct MountsInterface;

/* 
 * File Reader class intended to serve read calls via lowlevel MountsInterface and do 
 * not using standard POSIX toplevel interface. It is excluding zrt_zcall_*_read calls;
 */
struct MountsReader{
    ssize_t (*read)(struct MountsReader*, void *buf, size_t nbyte);
    /*private data*/
    int fd;                                   /*opened descriptor*/
    char*           buffer;                   /*buffer to be used for buffered io*/
    BufferedIORead* buffered_io_reader;       /*buffered io reader*/
    struct MountsInterface* mounts_interface; /*interface to filesystem*/
};

struct MountsReader* alloc_mounts_reader( struct MountsInterface* mounts_interface, const char* mounts_channel );
void free_mounts_reader( struct MountsReader* );

#endif /* MOUNTS_READER_H_ */
