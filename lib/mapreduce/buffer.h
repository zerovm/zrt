/*
 * buffer.h
 *
 *  Created on: 22.07.2012
 *      Author: yaroslav
 */

#ifndef __BUFFER_H_
#define __BUFFER_H_

#include <stddef.h> //size_t
#include <stdint.h> //uint32_t

#ifndef INLINE
# define INLINE static inline
#endif

typedef struct BufferHeader{
	size_t buf_size;
	size_t item_size;
	size_t count;
} BufferHeader;
typedef struct Buffer{
	BufferHeader header;
	char *data;
} Buffer;

void 
FreeBufferData(Buffer *buf);

/*@param buf pointer to unitialized Buffer struct
 *@param itemsize buffer item size
 *@param granularity memory reallocation chunk size
 *@return 0 if ok, otherwise on error unallocated data size*/
int 
AllocBuffer( Buffer *buf, int itemsize, uint32_t granularity );

/*@return 0 if ok, otherwise on error unallocated data size */
int 
ReallocBuffer( Buffer *buf );

/* INLINE void */
/* GetBufferItem(const Buffer *buf, int index, void *item); */

/* INLINE void */
/* SetBufferItem( Buffer *buf, int index, const void* ); */

/* INLINE void */
/* AddBufferItem( Buffer *buf, const void* item); */

/* INLINE const char * */
/* BufferItemPointer( const Buffer *buf, int index ); */

/* INLINE int */
/* BufferCountMax( const Buffer *buf ); */

#include "buffer.inl"

#endif /* __BUFFER_H_ */
