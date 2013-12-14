/*
 * Buffer array to be used by map-reduce library. 
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


#include <stdlib.h> //calloc
#include <string.h> //memcpy
#include <assert.h>
#include <alloca.h>

#include "buffer.h"


void FreeBufferData(Buffer *buf){
    buf->header.count = 0;
    buf->header.buf_size = 0;
    free( buf->data );
    buf->data = NULL;
}

int AllocBuffer( Buffer *buf, int itemsize, uint32_t granularity ){
    granularity = !granularity ? 1 : granularity;
    buf->header.item_size = itemsize;
    assert(itemsize>0);
    buf->header.count = 0;
    buf->header.buf_size = granularity * buf->header.item_size;
    buf->data = calloc( granularity, buf->header.item_size );
    if ( buf->data )
	return 0;
    else
	return granularity*buf->header.item_size; /*not allocated size*/
}

int ReallocBuffer( Buffer *buf ){
    assert(buf);
    assert(buf->header.item_size>0);
    if ( buf->header.count * buf->header.item_size == buf->header.buf_size ){
	buf->header.buf_size *= 2;
	buf->data = (char*)realloc( buf->data, buf->header.buf_size );
	if ( !buf->data )
	    return buf->header.buf_size; /*not allocated size*/
    }
    return 0;
}

