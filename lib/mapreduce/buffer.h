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
