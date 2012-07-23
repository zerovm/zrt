/*
 * buffer.c
 *
 *  Created on: 22.07.2012
 *      Author: yaroslav
 */

#include <stdlib.h> //calloc
#include <string.h> //memcpy
#include <assert.h>

#include "buffer.h"

void FreeBufferData(Buffer *buf){
	buf->header.count = 0;
	buf->header.buf_size = 0;
	free( buf->data );
	buf->data = NULL;
}

int AllocBuffer( Buffer *buf, DataType type, uint32_t granularity ){
	granularity = !granularity ? 1 : granularity;
	switch(type){
	case EByte:
		buf->header.item_size = 1;
		break;
	case EUint32:
		buf->header.item_size = sizeof(uint32_t);
		break;
	case EValue128:
		buf->header.item_size = 16;
		break;
	default:
		assert(0);
	}
	buf->header.data_type = type;
	buf->header.count = 0;
	buf->header.buf_size = granularity * buf->header.item_size;
	buf->data = calloc( granularity, buf->header.item_size );
	if ( buf->data )
		return 0;
	else
		return -1;
}

int ReallocBuffer( Buffer *buf ){
	if ( buf->header.count * buf->header.item_size == buf->header.buf_size ){
		buf->header.buf_size *= 2;
		buf->data = (char*)realloc( buf->data, buf->header.buf_size );
		if ( !buf->data )
			return -1;
	}
	return 0;
}

const char *GetBufferPointer( const Buffer *buf, int index ){
	return &buf->data[ buf->header.item_size * index ];
}

void GetBufferItem(const Buffer *buf, int index, void *item){
	memcpy( item, &buf->data[ buf->header.item_size * index ], buf->header.item_size  );
}

void SetBufferItem( Buffer *buf, int index, const void* item){
	memcpy( &buf->data[ buf->header.item_size * index ], (const char *)item, buf->header.item_size );
}

void AddBufferItem( Buffer *buf, const void* item){
	if ( buf->header.count == buf->header.buf_size / buf->header.item_size )
		ReallocBuffer(buf);
	memcpy( &buf->data[ buf->header.item_size * buf->header.count++ ], (const char *)item, buf->header.item_size );
}


