/*
 * buffer.c
 *
 *  Created on: 22.07.2012
 *      Author: yaroslav
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
	return -1;
}

int ReallocBuffer( Buffer *buf ){
    assert(buf);
    assert(buf->header.item_size>0);
    if ( buf->header.count * buf->header.item_size == buf->header.buf_size ){
	buf->header.buf_size *= 2;
	buf->data = (char*)realloc( buf->data, buf->header.buf_size );
	if ( !buf->data )
	    return -1;
    }
    return 0;
}

