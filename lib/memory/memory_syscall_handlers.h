/*
 * memory_syscall_handlers.h
 * Interface to memory manager for ZRT;
 *
 *  Created on: 23.10.2012
 *      Author: YaroslavLitvinov
 */

#ifndef __MEMORY_SYSCALL_HANDLERS_H__
#define __MEMORY_SYSCALL_HANDLERS_H__

#include <stddef.h> //size_t 

#define MAX_MEMORY_CAPACITY_IN_GB 4
#define ONE_GB_HEAP_SIZE (1024*1024*1024)
#define PAGE_SIZE (1024*64)
#define MAX_MMAP_PAGES_COUNT ( MAX_MEMORY_CAPACITY_IN_GB*(ONE_GB_HEAP_SIZE/PAGE_SIZE) )

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define NO_ADDR_OVERLAP(low_addr, high_addr) (low_addr) < (high_addr) ? 1 : 0
#define HEAP_MAX_ADDR(heapptr_p, heapsize)  (heapptr_p+heapsize)

#define MMAP_REGION_SIZE_ALIGNED_(heapptr_p, brk_p, heapsize)	\
    (ROUND_UP(heapsize - ((brk_p) - (heapptr_p)), PAGE_SIZE))

#define MMAP_REGION_SIZE_ALIGNED(memory_if_p)			\
    MMAP_REGION_SIZE_ALIGNED_(memory_if_p->heap_start_ptr,	\
			      memory_if_p->heap_brk,		\
			      memory_if_p->heap_size )

#define MMAP_LOWEST_PAGE_ADDR(memory_if_p)				\
    (HEAP_MAX_ADDR(memory_if_p->heap_start_ptr, memory_if_p->heap_size) - MMAP_REGION_SIZE_ALIGNED(memory_if_p))

#define MMAP_HIGHEST_PAGE_ADDR(memory_if_p)				\
    (HEAP_MAX_ADDR(memory_if_p->heap_start_ptr, memory_if_p->heap_size) - PAGE_SIZE)


/* Low level memory management functions, here are syscalls
 * implementation.  Note that all member functions has
 * 'MemoryInterface* this' param, and it can't be a NULL, because it
 * used as 'this' pointer and need to access data that object hold,
 * also it's makes available to call another object functions;*/
struct MemoryInterface{
    /*Low level memory allocators initializer.  Whole heap memory
     already preallocated/mmaped by zerovm, it is happened just before
     untrusted session starts. As we have a single address space
     for both sbrk, mmap allocators we need to avoid intersection of
     sbrk and mmap memory ranges.  Strategy is : sbrk range starting
     from beginning of heap memory and mmap range starting from end of
     memory range. At the moment of allocation if ranges are become
     overlaped then get ENOMEM errro; 
     @param heap_ptr 
     @param heap_size */
    void (*init)(struct MemoryInterface* this, void *heap_ptr, uint32_t heap_size, void *brk);
    int (*sysbrk)(struct MemoryInterface* this, void *addr);
    
    /* MMAP emulation in user-space implementation.
     * @param addr ignored
     * @param length length of the mapping, it's used when fd value is not exist
     * and MAP_ANONYMOUS flag is set;
     * @prot 
     * case1: if correct fd values is passed then PROT_READ only supported, another 
     * prot flags will be ignored; it's implemented by simple loading of whole file
     * contents into memory, so memory always mapping file contents,
     * case2: if fake fd value passed and MAP_ANONYMOUS flag is set then prot flags
     * PROT_READ|PROT_WRITE both supported and here requeted memory range will allocated,
     * case3: for any another prot flag will returned error, and ENOSYS set to errno;
     * @param flags see above MAP_ANONYMOUS flag using;
     */
    int32_t (*mmap)(struct MemoryInterface* this, void *addr, size_t length, int prot, 
		    int flags, int fd, off_t offset);
    
    /* MUNMAP emulation in user-space implementation.
     * @param addr should be actual address starting map region
     * @param length ignored  
     * unmap memory range, in practice memory will released at specified addr*/
    int (*munmap)(struct MemoryInterface* this, void *addr, size_t length);

    //data
    void*    heap_start_ptr; /*heap memory left bound*/
    void*    heap_brk;       /*current brk pointer*/
    uint32_t heap_size;      /*entire heap size*/
    void*    heap_lowest_mmap_addr;
};

struct MemoryInterface* get_memory_interface( void *heap_ptr, uint32_t heap_size, void *brk );

#endif //__MEMORY_SYSCALL_HANDLERS_H__
