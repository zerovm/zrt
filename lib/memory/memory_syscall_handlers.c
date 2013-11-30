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
#include "bitarray.h"

#define MEMORY_PAGE_FROM_INDEX(memory_if_p, index) ( MMAP_HIGHEST_PAGE_ADDR(memory_if_p) - index*PAGE_SIZE)

/***************mmap emulation *************/

/*map_chunks_bit_array is used for bitarray, were are only 1bit per
 *1page needed.  here reserved memory for max available pages count.*/
static unsigned char s_map_chunks_bit_array[(MAX_MMAP_PAGES_COUNT)/8]; 
static struct BitArrayImplem s_bit_array_implem;
static const uint32_t s_page_size = PAGE_SIZE;


static void* alloc_memory_pseudo_mmap(struct BitArrayImplem* bit_array_implem, 
				      struct MemoryInterface* mem_if_p, 
				      size_t memsize){ 
    int i;
    void* ret_addr = NULL;
    int map_pages_requested = ROUND_UP(memsize, s_page_size)/s_page_size;

    /*search for sequence of free pages starting from end*/
    int mmap_index = bit_array_implem
	->search_emptybit_sequence_begin(bit_array_implem, map_pages_requested); 
    int mmap_low_page_index = mmap_index+ map_pages_requested-1;

    /*if indexes of map pages seems to be valid*/
    if ( mmap_index != -1 ){	
	void* high_page_memory_addr = MEMORY_PAGE_FROM_INDEX(mem_if_p, mmap_index);
	void* low_page_memory_addr = MEMORY_PAGE_FROM_INDEX(mem_if_p, mmap_low_page_index);
	ZRT_LOG(L_BASE, "mmap_index=%d, mmap_low_page_index=%d", 
		mmap_index, mmap_low_page_index);
	ZRT_LOG(L_BASE, "pages low_addr=%p, high_addr=%p ", 
		low_page_memory_addr, high_page_memory_addr);
	/*it's only allowed to return mmap pages which address upper
	 *than brk pointer; */
	if ( low_page_memory_addr >= mem_if_p->heap_brk &&
	     high_page_memory_addr <= MEMORY_PAGE_FROM_INDEX(mem_if_p, mmap_index) ){
	    ret_addr = low_page_memory_addr;
	    /*additional checks*/
	    assert( ret_addr >= mem_if_p->heap_brk );
	    assert(low_page_memory_addr <= high_page_memory_addr);
	    
	    /*mark map chunks corresponding to returning block of memory*/
	    int current_chunk_index;
	    for ( i=0; i < map_pages_requested; i++ ){
		current_chunk_index = mmap_index+i;
		LOG_DEBUG(ELogIndex, current_chunk_index, "mark chunk as used" );
		bit_array_implem->toggle_bit(bit_array_implem, current_chunk_index);
	    }
	    ZRT_LOG(L_SHORT, "PSEUDO_MMAP(%u) ret addr=%p", memsize, ret_addr ); 
	}
    }
    else{
	ZRT_LOG(L_SHORT, "PSEUDO_MMAP(%u) failed", memsize );
    }
    return ret_addr;
}


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
memory_init(struct MemoryInterface* this, 
	    void *heap_ptr, uint32_t heap_size, void *brk){
    this->heap_start_ptr = heap_ptr;
    this->heap_size = heap_size;
    this->heap_brk = brk;  /*current brk*/
    this->heap_lowest_mmap_addr = MMAP_HIGHEST_PAGE_ADDR(this);
}


/**/
static int32_t 
memory_sysbrk(struct MemoryInterface* this, void *addr){
    /*if requested addr is in range of available heap range then it's
      would be returned as heap bound*/
    if ( addr < this->heap_start_ptr ){
	errno=0;
	return (intptr_t)this->heap_brk; /*return current brk*/
    }
    /*sbrk does not intersect mmap region*/
    else if ( addr < this->heap_lowest_mmap_addr ){
	errno=0;
	this->heap_brk = addr;
	return (intptr_t)this->heap_brk;
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
	    /*min amount of memory here is PAGE_SIZE*/
	    wanted_mem_block_size = st.st_size == 0 ? PAGE_SIZE : st.st_size;
	    alloc_addr = alloc_memory_pseudo_mmap( &s_bit_array_implem, this, wanted_mem_block_size );
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
			(long long int)mmap_data_len, (long long int)st.st_size );
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
	alloc_addr = alloc_memory_pseudo_mmap( &s_bit_array_implem, this, wanted_mem_block_size );
	if ( alloc_addr != NULL )
	    errcode = 0;
	else
	    errcode = ENOMEM;
    } 
    /*if was supplied unsupported prot or flags params return -1; */
    else{
	/*requested features not supported by implementation*/
	errcode = ENOSYS;
    }
    
    if ( errcode == 0 ){
	/*update lowest mmap addr, for future check sbrk/mmap intersections*/
	if ( alloc_addr < this->heap_lowest_mmap_addr )
	    this->heap_lowest_mmap_addr = alloc_addr;
	return (intptr_t)alloc_addr;
    }
    else{
	errno = errcode;
	return (intptr_t)MAP_FAILED;
    }
}

static int
memory_munmap(struct MemoryInterface* this, void *addr, size_t length){
    errno=0;
    /*if requested addr is in range of available heap range then it's
      would be returned as heap bound*/
    if ( addr >= MMAP_LOWEST_PAGE_ADDR(this) && addr <= MMAP_HIGHEST_PAGE_ADDR(this) ){
	int i;
	int munmap_begin_chunk_index = (MMAP_HIGHEST_PAGE_ADDR(this) - addr) / s_page_size;
	int munmap_chnuks_count = ROUND_UP(length,PAGE_SIZE)/PAGE_SIZE;
	LOG_DEBUG(ELogIndex, munmap_begin_chunk_index, "" );
	LOG_DEBUG(ELogCount, munmap_chnuks_count, "" );

	/*unmap region chunk by chunk*/
	for ( i=munmap_begin_chunk_index; i < munmap_begin_chunk_index+munmap_chnuks_count ; i++ ){
	    int bit = s_bit_array_implem.get_bit( &s_bit_array_implem, i);
	    ZRT_LOG(L_SHORT, "bit index=%d value=%d", i, bit);
	    if ( bit == 1 ){
		s_bit_array_implem.toggle_bit( &s_bit_array_implem, i);
	    }
	    else{
		LOG_DEBUG(ELogAddress, addr, "indicated address does not contain maped region" )
		    }
	}
    }
    else{
	ZRT_LOG(L_ERROR, "Can't do unmap addr=%p is not in range [%p-%p]", 
		addr, MMAP_LOWEST_PAGE_ADDR(this), MMAP_HIGHEST_PAGE_ADDR(this) );
    }
    return 0;
}


static struct MemoryInterface KMemoryInterface = {
    memory_init,
    memory_sysbrk,
    memory_mmap,
    memory_munmap
};

struct MemoryInterface* get_memory_interface( void *heap_ptr, uint32_t heap_size, void *brk ){
    /*round up tests*/
    assert(roundup_pow2( 0 )== 0);
    assert(roundup_pow2( 4 )== 4);
    assert(roundup_pow2( 63 )== 64);
    assert(roundup_pow2( 66 )== 128);
    assert(roundup_pow2( 536860612 )== 536870912);
    /*check if we have valid PAGE_SIZE macro*/
    assert( s_page_size == PAGE_SIZE );
    /*check align for right bound of heap*/
    uint32_t heap_max_addr = (uint32_t)HEAP_MAX_ADDR(heap_ptr,heap_size);
    assert( heap_max_addr == ROUND_UP(heap_max_addr, s_page_size) );

    int mmap_pages_count = MMAP_REGION_SIZE_ALIGNED_(heap_ptr, brk, heap_size) / s_page_size;

    /*init array used to store status of chunks for mmap/munmap.
     *keep in mind that pages quantity can be not multiple on 8*/
    int bit_array_size = mmap_pages_count/8; bit_array_size += mmap_pages_count%8>0?1:0;
    LOG_DEBUG(ELogSize, bit_array_size, "bitarray actual size in bytes" );

    init_bit_array( &s_bit_array_implem, s_map_chunks_bit_array, bit_array_size );

    KMemoryInterface.init(&KMemoryInterface, heap_ptr, heap_size, brk);
    return &KMemoryInterface;
}
