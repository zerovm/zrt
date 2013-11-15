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

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))


/***************mmap emulation *************/
#define MAX_HEAP_SIZE 1024*1024*1024*4
#define PAGE_SIZE (1024*64)
static struct BitArrayImplem s_bit_array_implem;
static unsigned char map_chunks_bit_array[MAX_HEAP_SIZE/PAGE_SIZE/8];

static void* alloc_memory_pseudo_mmap(struct BitArrayImplem* bit_array_implem, 
				      struct MemoryInterface* mem_if_p, 
				      size_t memsize){ 
    int i;
    void* ret_addr = NULL;
    int right_index_bound = mem_if_p->heap_size / PAGE_SIZE;
    int map_chunks_sequence_count = ROUND_UP(memsize, PAGE_SIZE)/PAGE_SIZE;
    int index = bit_array_implem
	->search_emptybit_sequence_begin(bit_array_implem, map_chunks_sequence_count ); 

    if ( index != -1 && index < right_index_bound ){	
	/*addr of block memory must be addr of first chunk*/
	ret_addr = mem_if_p->heap_ptr + index*PAGE_SIZE;
	assert( ret_addr < mem_if_p->heap_ptr+mem_if_p->heap_size );

	/*set chunk bits corresponding to block of memory*/
	for ( i=0; i < map_chunks_sequence_count; i++ ){
	    int current_chunk_index = index+i;
	    LOG_DEBUG(ELogIndex, current_chunk_index, "mark chunk as used" );
    	    bit_array_implem->toggle_bit(bit_array_implem, current_chunk_index);
	}
	ZRT_LOG(L_SHORT, "PSEUDO_MMAP(%lu) ret addr=%p", memsize, ret_addr ); 
    }
    else{
	ZRT_LOG(L_SHORT, "PSEUDO_MMAP(%lu) failed", memsize );
    }
    if ( ret_addr == 0xef90000 ){
	int r = 0;
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
    void* end_range   = this->heap_ptr+this->heap_size;
    /*if requested addr is in range of available heap range then it's
      would be returned as heap bound*/
    if ( addr >= this->heap_ptr && addr <= end_range ){
	int i;
	int c = addr - this->heap_ptr;
	int munmap_begin_chunk_index = (addr - this->heap_ptr)/PAGE_SIZE;
	int munmap_chnuks_count = ROUND_UP(length,PAGE_SIZE)/PAGE_SIZE;
	LOG_DEBUG(ELogIndex, munmap_begin_chunk_index, "" )
	LOG_DEBUG(ELogCount, munmap_chnuks_count, "" )

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
		addr, this->heap_ptr, end_range );
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

    /*If we want to manage pages of heap memory it's required to roundup heap_ptr
      because it's not aligned to page boundary, recalculate heap size and validate it*/
    uint32_t addr_uint =  ROUND_UP( (uint32_t)heap_ptr, sysconf(_SC_PAGESIZE) );
    heap_size -= addr_uint - (uint32_t)heap_ptr;
    assert( ROUND_UP( (uint32_t)addr_uint, sysconf(_SC_PAGESIZE) ) == addr_uint );
    assert( ROUND_UP( (uint32_t)heap_size, sysconf(_SC_PAGESIZE) ) == heap_size );
    /*check if we have valid PAGE_SIZE macro*/
    assert( sysconf(_SC_PAGESIZE) == PAGE_SIZE );

    ZRT_LOG(L_BASE, "Mmap memory is: [1st page addr=0x%x, last page addr=0x%x, size=0x%x, pages count=%d]", 
	    addr_uint, addr_uint+heap_size-sysconf(_SC_PAGESIZE), heap_size, heap_size/PAGE_SIZE );

    /*init array used to store status of chunks for mmap/munmap*/
    int array_items_count = heap_size/PAGE_SIZE/8;
    array_items_count += (heap_size/PAGE_SIZE)%8>0?1:0;

    LOG_DEBUG(ELogSize, array_items_count, "bitarray actual size in bytes" )
    assert(array_items_count*8 >=heap_size/PAGE_SIZE);

    init_bit_array( &s_bit_array_implem, map_chunks_bit_array, array_items_count );

    KMemoryInterface.init(&KMemoryInterface, (void*)addr_uint, heap_size);
    return &KMemoryInterface;
}
