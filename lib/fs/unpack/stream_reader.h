/*
 * stream_reader.h
 *
 *  Created on: 27.09.2012
 *      Author: yaroslav
 */

#ifndef STREAM_READER_H_
#define STREAM_READER_H_

#include <unistd.h> //ssize_t

struct MountsInterface;

struct StreamReader{
    ssize_t (*read)(struct StreamReader*, void *buf, size_t nbyte);
    //data
    int fd; /*opened descriptor*/
    struct MountsInterface* mounts_interface; /*interface to readable filesystem*/
};

struct StreamReader* alloc_stream_reader( struct MountsInterface* mounts_interface, const char* stream_channel );
void free_stream_reader( struct StreamReader* );

#endif /* STREAM_READER_H_ */
