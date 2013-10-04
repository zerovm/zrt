
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "zrtlog.h"
#include "buffered_io.h"
					

#define WRITE_IF_BUFFER_ENOUGH(iowrite, data, size)			\
    if ( size <= iowrite->data.bufmax - iowrite->data.cursor ){		\
	/*buffer is enough to write data*/				\
	memcpy( iowrite->data.buf + iowrite->data.cursor, data, size );	\
	iowrite->data.cursor += size;					\
    }

#define READ_IF_BUFFER_ENOUGH(ioread, data, size)			\
    if ( size <= ioread->buffered(ioread) ){				\
	/*buffer is enough to read data*/				\
	memcpy( data, ioread->data.buf + ioread->data.cursor, size );	\
	ioread->data.cursor += size;					\
    }


void buf_flush_write( BufferedIOWrite* self, int handle ){
    if ( self->data.cursor ){
	int b = self->write_override(handle, self->data.buf, self->data.cursor);
	ZRT_LOG(L_EXTRA, "buffered_io: flush %d/%d bytes \n", b, self->data.cursor );
	self->data.cursor = 0;
    }
}

int buf_write(BufferedIOWrite* self, int handle, const void* data, size_t size ){
    WRITE_IF_BUFFER_ENOUGH(self, data, size)
    else{
	self->flush_write(self, handle);
	WRITE_IF_BUFFER_ENOUGH(self, data, size)
	else{
	    /*write directly to fd, buffer is too small*/  
	    int b = self->write_override(handle, data, size);
	    ZRT_LOG(L_EXTRA, "buffered_io: wrote %d/%d bytes into file \n", b, size );
	    return b;
	}
    }
    return size;
}

int buf_buffered(struct BufferedIORead* self){
    return self->data.datasize - self->data.cursor;
}

int buf_read (BufferedIORead* self, int handle, void* data, size_t size){
    READ_IF_BUFFER_ENOUGH(self, data, size)
    else{
	int sizeinuse=self->buffered(self);
	/*move unread data into beginning, it's overlap safe*/
	if ( sizeinuse > 0 ){
	    memmove(self->data.buf, 
		    self->data.buf + self->data.cursor, 
		    sizeinuse );
	    ZRT_LOG(L_EXTRA, "buffered_io: moved %d bytes to be read \n", sizeinuse );
	}
	self->data.cursor = 0;
	/*read into buffer from file descriptor*/  
	int bytes = self->read_override(handle, self->data.buf + sizeinuse, self->data.bufmax-sizeinuse );
	if ( bytes > 0 ){
	    self->data.datasize = sizeinuse+bytes;
	    ZRT_LOG( L_EXTRA, "buffered_io: read to buffer %d bytes \n", bytes );
	    READ_IF_BUFFER_ENOUGH(self, data, size)
	    else{
		/*insufficient buffer space, so read part of data from buffer
		  and rest of data directly from handle*/
		int cached = self->buffered(self);
		READ_IF_BUFFER_ENOUGH(self, data, cached );
		bytes = self->read_override(handle, data+cached, size-cached);
		ZRT_LOG( L_EXTRA, "buffered_io: direct read %d/%d bytes \n", bytes, size );
	    }
	}
	else{
	    /*eof*/
	    return -1;
	}
    }
    return size;
}


BufferedIOWrite* AllocBufferedIOWrite(void* buf, size_t size,
				      ssize_t (*f) (int handle, const void* data, size_t size) ){
    ZRT_LOG( L_SHORT, "AllocBufferedIOWrite buf=%p, size=%d \n", buf, size );
    assert(buf);
    BufferedIOWrite* self = malloc( sizeof(BufferedIOWrite) );
    self->data.buf = buf;
    self->data.bufmax = size;
    self->data.cursor=0;
    self->flush_write = buf_flush_write;
    self->write = buf_write;
    /*we can override std write function if passed function not NULL */
    if ( f )
	self->write_override = f;
    else
	self->write_override = write;
    return self;
}

BufferedIORead* AllocBufferedIORead(void* buf, size_t size,
				    ssize_t (*f) (int handle, void* data, size_t size) ){
    ZRT_LOG( L_INFO, "AllocBufferedIORead buf=%p, size=%d \n", buf, size );
    assert(buf);
    BufferedIORead* self = malloc( sizeof(BufferedIORead) );
    self->data.buf = buf;
    self->data.bufmax = size;
    self->data.cursor = 0;
    self->data.datasize = 0;
    self->buffered = buf_buffered;
    self->read  = buf_read;
    /*we can override std read function if passed function not NULL */
    if ( f )
	self->read_override = f;
    else
	self->read_override = read;
    return self;
}
