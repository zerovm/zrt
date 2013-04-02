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
    struct BufferedIOData data;
} BufferedIOWrite;

typedef struct BufferedIORead{
    int  (*read) (struct BufferedIORead* self, int handle, void* data, size_t size);
    int  (*buffered)(struct BufferedIORead* self);
    struct BufferedIOData data;
} BufferedIORead;



/*Create io engine with buffering facility, i/o optimizer.
  alloc in heap, ownership is transfered*/
BufferedIOWrite* AllocBufferedIOWrite(void* buf, size_t size);
BufferedIORead* AllocBufferedIORead(void* buf, size_t size);


#endif //__BUFFERED_IO_H__

