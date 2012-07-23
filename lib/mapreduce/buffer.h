/*
 * buffer.h
 *
 *  Created on: 22.07.2012
 *      Author: yaroslav
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include <stddef.h> //size_t
#include <stdint.h> //uint32_t

typedef enum { EByte, EUint32, EValue128 } DataType;
typedef struct BufferHeader{
	size_t buf_size;
	DataType data_type;
	size_t item_size;
	size_t count;
} BufferHeader;
typedef struct Buffer{
	BufferHeader header;
	char *data;
} Buffer;

void FreeBufferData(Buffer *buf);
/*@return 0 if ok, otherwise -1 */
int AllocBuffer( Buffer *buf, DataType type, uint32_t granularity );
/*@return 0 if ok, otherwise -1 */
int ReallocBuffer( Buffer *buf );
void GetBufferItem(const Buffer *buf, int index, void *item);
void SetBufferItem( Buffer *buf, int index, const void* );
void AddBufferItem( Buffer *buf, const void* item);
const char *GetBufferPointer( const Buffer *buf, int index );

#endif /* BUFFER_H_ */
