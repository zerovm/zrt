/*
 * memory_syscall_handlers.c
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
#include "memory_syscall_handlers.h"

#define USE_MALLOC

/*memory allocator uses posix_memalign for big allocations*/
#define ALLOC_MMAP_MEMORY_POSIX_MEMALIGN(addr_p, memsize){		\
	size_t rounded_memsize = sysconf(_SC_PAGESIZE);			\
    	/* size_t rounded_memsize = roundup_pow2(memsize); */		\
	int err = posix_memalign(addr_p, rounded_memsize, memsize);	\
	if ( err != 0 ){						\
	    SET_ERRNO(err);						\
	    ZRT_LOG(L_SHORT, "warning: posix_memalign(%p,alignment=%u,size=%lld) failed", \
		    *addr_p, rounded_memsize, memsize  );		\
	}								\
	else{								\
	    ZRT_LOG(L_SHORT, "retaddr %p posix_memalign alignment=%u, size=%lld", \
		    *addr_p, rounded_memsize, memsize  );		\
	}								\
    }

/*simple memory allocator uses malloc for big allocations*/
#define ALLOC_MMAP_MEMORY_MALLOC(addr_p, memsize){			\
	if ( ( *addr_p = malloc( memsize ) ) == NULL ){			\
	    SET_ERRNO(errno);						\
	    ZRT_LOG(L_SHORT, "malloc(%lld) failed", memsize );		\
	}								\
	else{								\
	    ZRT_LOG(L_SHORT, "malloc(%lld) ret addr=%p", memsize, *addr_p ); \
	}								\
    }

#ifdef USE_MALLOC
#  define MIN_MMAP_MEMORY_SIZE 1
#  define ALLOC_MMAP_MEMORY(addr_p, memsize) ALLOC_MMAP_MEMORY_MALLOC(addr_p, memsize)
#else
#  define MIN_MMAP_MEMORY_SIZE 100
#  define ALLOC_MMAP_MEMORY(addr_p, memsize) ALLOC_MMAP_MEMORY_POSIX_MEMALIGN(addr_p, memsize)
#endif


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
    this->brk = heap_ptr;  /*brk by default*/
}


/**/
static int32_t 
sysbrk(struct MemoryInterface* this, void *addr){
    /*if requested addr is in range of available heap range then it's
      would be returned as heap bound*/
    if ( addr < this->heap_ptr ){
	errno=0;
	return (intptr_t)this->brk; /*return current brk*/
    }
    else if ( addr <= this->heap_ptr+this->heap_size ){
	errno=0;
	this->brk = addr;
	return (intptr_t)this->brk;
    }
    else{
	SET_ERRNO(ENOMEM);
	return -1;
    }
}

static int32_t
memory_mmap(struct MemoryInterface* this, void *addr, size_t length, int prot, 
     int flags, int fd, off_t offset){
    void* alloc_addr = NULL;
    int errcode = 0;
    off_t wanted_mem_block_size = length;
    /* check for allowed case, if prot supplied at least with PROT_READ and fd param
     * passed seems to be correct then try to map file into memory*/
    if ( CHECK_FLAG(prot, PROT_READ) && fd > 0 ){
	/*doing simple mmap, get stat for file descriptor, allocate memory
	 and just read file into memory*/
	struct stat st;
	/*fstat is checking fd passed and get file size*/
	if ( !fstat(fd, &st) ){
	    if ( st.st_size == 0 ){
		/*min allocated memory size*/
		wanted_mem_block_size = MIN_MMAP_MEMORY_SIZE;
	    }
	    /*use allocation of non alligned memory*/
	    ALLOC_MMAP_MEMORY_MALLOC( &alloc_addr, wanted_mem_block_size );
	    if ( alloc_addr != NULL ){
		errcode = 0;
		/*create maping in memory just copying file contents starting from offset
		 *synchronizing data with offset in memory, so at first get current file
		 *offset and check with passed offset and if that equals then not do seek
		 *to the required offset(it's suitable for sequential accessed files)*/
		off_t current_offset = lseek(fd, 0, SEEK_CUR);
		assert(current_offset!=-1);
		if ( current_offset != offset ){
		    current_offset = lseek(fd, offset, SEEK_SET);
		    assert(current_offset!=-1);
		}
		/*read into memory starting with offest*/
		ssize_t mmap_data_len = st.st_size-offset;
		ssize_t copied_bytes = read( fd, alloc_addr+offset, mmap_data_len );
		/*file size returned by fstat should be the same as readed bytes count*/
		assert( copied_bytes == mmap_data_len );
		ZRT_LOG(L_INFO, "file mmap OK, mmap_data_len=%lld, filesize=%lld", 
			mmap_data_len, st.st_size );
	    }
	    else{
		errcode = errno;
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
	/*use allocation of alligned memory*/
	ALLOC_MMAP_MEMORY_POSIX_MEMALIGN( &alloc_addr, wanted_mem_block_size );
	if ( alloc_addr != NULL )
	    errcode = 0;
	else
	    errcode = errno;
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
    errno=0;
    void* begin = this->heap_ptr;
    void* end   = this->heap_ptr+this->heap_size;
    /*if requested addr is in range of available heap range then it's
      would be returned as heap bound*/
    if ( !(addr >= this->heap_ptr && addr <= this->heap_ptr+this->heap_size) ){
	ZRT_LOG(L_ERROR, "addr=%p is not in range [%p-%p]", addr, begin, end );
	free(addr);
    }
    return 0;
}


static struct MemoryInterface KMemoryInterface = {
    init,
    sysbrk,
    memory_mmap,
    memory_munmap
};

struct MemoryInterface* get_memory_interface( void *heap_ptr, uint32_t heap_size ){
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
