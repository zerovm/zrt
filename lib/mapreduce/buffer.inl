
#ifndef __BUFFER_INL__
#define __BUFFER_INL__

INLINE int
BufferCountMax( const Buffer *buf ){
    return buf->header.item_size>0? buf->header.buf_size / buf->header.item_size:0;
}

INLINE const char *
BufferItemPointer( const Buffer *buf, int index ){
    return &buf->data[ buf->header.item_size * index ];
}

INLINE void 
GetBufferItem(const Buffer *buf, int index, void *item){
    memcpy( item, BufferItemPointer( buf, index ), buf->header.item_size );
}

INLINE void 
SetBufferItem( Buffer *buf, int index, const void* item){
    memcpy( (char*)BufferItemPointer( buf, index ), 
            (const char *)item, 
	    buf->header.item_size );
}

INLINE void 
AddBufferItem( Buffer *buf, const void* item){
    if ( buf->header.count == BufferCountMax( buf ) ){
	if ( ReallocBuffer(buf) ){
	    assert(0);
	}	
    }
    SetBufferItem( buf, buf->header.count++, (const char *)item);
}

/*guarantied reserve space for item and get item index*/
INLINE int 
AddBufferItemVirtually( Buffer *buf){
    if ( buf->header.count == BufferCountMax( buf ) ){
	if ( ReallocBuffer(buf) ){
	    assert(0);
	}
    }
    return buf->header.count++;
}


#endif //__BUFFER_INL__


