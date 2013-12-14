/*
 *
 * Copyright (c) 2013, LiteStack, Inc.
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

#ifndef __BUFFERED_IO_H__
#define __BUFFERED_IO_H__

#include <stddef.h> //size_t 

struct BufferedIOData{
    char* buf;
    int   bufmax;
    int   cursor;
    // for read
    int   datasize;
};

typedef struct BufferedIOWrite{
    void (*flush_write)(struct BufferedIOWrite* self, int handle);
    int  (*write)(struct BufferedIOWrite* self, int handle, const void* data, size_t size);
    ssize_t (*write_override) (int handle, const void* data, size_t size);
    struct BufferedIOData data;
} BufferedIOWrite;

typedef struct BufferedIORead{
    int  (*read) (struct BufferedIORead* self, int handle, void* data, size_t size);
    int  (*buffered)(struct BufferedIORead* self);
    ssize_t  (*read_override) (int handle, void* data, size_t size);
    struct BufferedIOData data;
} BufferedIORead;



/*Create io engine with buffering facility, i/o optimizer.
  alloc in heap, ownership is transfered*/
BufferedIOWrite* AllocBufferedIOWrite(void* buf, size_t size,
				      ssize_t (*write) (int handle, const void* data, size_t size) );

BufferedIORead*  AllocBufferedIORead(void* buf, size_t size,
				     ssize_t (*read) (int handle, void* data, size_t size) );


#endif //__BUFFERED_IO_H__

