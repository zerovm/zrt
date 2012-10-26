/*
 * memory.c
 * Interface to memory manager for ZRT;
 *
 *  Created on: 23.10.2012
 *      Author: YaroslavLitvinov
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> //posix_memalign
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "memory.h"

/*return aligned value that multiple of the power of 2*/
static size_t roundup_pow2(size_t value){
    size_t bit_mask_after_hi(size_t x);
    return bit_mask_after_hi(value-1) + 1;
}

size_t bit_mask_after_hi(size_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x;
}

static void
init(struct MemoryInterface* this, void *heap_ptr, uint32_t heap_size){
    this->heap_ptr = heap_ptr;
    this->heap_size = heap_size;
}


/**/
static int32_t 
sysbrk(struct MemoryInterface* this, void *addr){
    /*if requested addr is in range of available heap range then it's
      would be returned as heap bound*/
    if ( addr >= this->heap_ptr && addr <= this->heap_ptr+this->heap_size ){
	errno=0;
	return (intptr_t)addr;
    }
    else{
	errno=ENOMEM;
	ZRT_LOG_ERRNO(errno);
	return -1;
    }
}

static int32_t
memory_mmap(struct MemoryInterface* this, void *addr, size_t length, int prot, 
     int flags, int fd, off_t offset){
    void* alloc_addr = NULL;
    int errcode = 0;
    /* check for allowed case, if prot supplied with PROT_READ and fd param
     * passed seems to be correct then try to map file into memory*/
    if ( CHECK_FLAG(prot, PROT_READ) && fd > 0 ){
	/*doing simple mmap, get stat for file descriptor, allocate memory
	 and just read file into memory*/
	struct stat st;
	/*fstat is checking fd passed and get file size*/
	if ( !fstat(fd, &st) ){
	    errcode = posix_memalign(&alloc_addr, 
					 roundup_pow2(st.st_size), st.st_size);
	    if ( errcode == 0 ){
		ssize_t copied_bytes = read( fd, alloc_addr, st.st_size );
		/*file size returned by fstat should be the same as readed bytes count*/
		assert( copied_bytes == st.st_size );
	    }
	}
	/*fstat fail with passed fd value*/
	else{
	    /*specified file sdescriptor not available*/
	    errcode = EBADF;
	}
    }
    /*if anonymous mapping requested then do it*/
    else if ( CHECK_FLAG(prot, PROT_READ|PROT_WRITE) &&
	      CHECK_FLAG(flags, MAP_ANONYMOUS) && length >0 ){
	errcode = posix_memalign(&alloc_addr, roundup_pow2(length), length);
    } 
    /*if was supplied unsupported prot or flags params return -1; */
    else{
	/*requested features not supported by implementation*/
	errcode = ENOSYS;
    }

    if ( errcode == 0 )
	return (intptr_t)alloc_addr;
    else{
	errno = errcode;
	return (intptr_t)MAP_FAILED;
    }
}

static int
memory_munmap(struct MemoryInterface* this, void *addr, size_t length){
    /*if requested addr is in range of available heap range then it's
      would be returned as heap bound*/
    if ( addr >= this->heap_ptr && addr <= this->heap_ptr+this->heap_size ){
	free(addr);
	errno=0;
	return 0;
    }
    else{
	errno=EINVAL;
	ZRT_LOG_ERRNO(errno);
	return -1;
    }
}


static struct MemoryInterface KMemoryInterface = {
    init,
    sysbrk,
    memory_mmap,
    memory_munmap
};

struct MemoryInterface* memory_interface( void *heap_ptr, uint32_t heap_size ){
    /*round up tests*/
    assert(roundup_pow2( 0 )== 0);
    assert(roundup_pow2( 4 )== 4);
    assert(roundup_pow2( 63 )== 64);
    assert(roundup_pow2( 66 )== 128);
    assert(roundup_pow2( 536860612 )== 536870912);
    /**/

    KMemoryInterface.init(&KMemoryInterface, heap_ptr, heap_size);
    return &KMemoryInterface;
}
