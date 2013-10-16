/*
 * mounts_reader.h
 *
 *  Created on: 27.09.2012
 *      Author: yaroslav
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
