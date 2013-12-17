/*
 * memory_syscall_handlers.c
 * Interface to memory manager for ZRT;
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

#ifndef __MEMORY_SYSCALL_HANDLERS_H__
#define __MEMORY_SYSCALL_HANDLERS_H__

#include <stddef.h> //size_t 

#include "bitarray.h"

#include "zrt_defines.h" //CONSTRUCT_L

#define MAX_MEMORY_CAPACITY_IN_GB 4
#define ONE_GB_HEAP_SIZE (1024*1024*1024)
#define PAGE_SIZE (1024*64)
#define MAX_MMAP_PAGES_COUNT ( MAX_MEMORY_CAPACITY_IN_GB*(ONE_GB_HEAP_SIZE/PAGE_SIZE) )

/*name of constructor*/
#define MEMORY_MANAGER memory_interface_construct


/* Low level memory management functions, here are syscalls
 * implementation.  Note that all member functions has
 * 'MemoryManagerPublicInterface* this' param, and it can't be a NULL, because it
 * used as 'this' pointer and need to access data that object hold,
 * also it's makes available to call another object functions;*/


struct MemoryManagerPublicInterface{
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
    void (*init)(struct MemoryManagerPublicInterface* this, void *heap_ptr, uint32_t heap_size, void *brk);
    int (*sysbrk)(struct MemoryManagerPublicInterface* this, void *addr);
    
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
    int32_t (*mmap)(struct MemoryManagerPublicInterface* this, void *addr, size_t length, int prot, 
		    int flags, int fd, off_t offset);
    
    /* MUNMAP emulation in user-space implementation.
     * @param addr should be actual address starting map region
     * @param length ignored  
     * unmap memory range, in practice memory will released at specified addr*/
    int (*munmap)(struct MemoryManagerPublicInterface* this, void *addr, size_t length);

    /*result for sysconf(_SC_PHYS_PAGES)*/
    long int (*get_phys_pages)(struct MemoryManagerPublicInterface* this);
    /*result for sysconf(_SC_AVPHYS_PAGES)*/
    long int (*get_avphys_pages)(struct MemoryManagerPublicInterface* this);
};

struct MemoryManager{
    //base, it is must be a first member
    struct MemoryManagerPublicInterface public;

    //data
    void*    heap_start_ptr; /*heap memory left bound*/
    void*    heap_brk;       /*current brk pointer*/
    uint32_t heap_size;      /*entire heap size*/
    void*    heap_lowest_mmap_addr;
    //
    /*map_chunks_bit_array is used for bitarray, were are only 1bit per
     *1page needed.  here reserved memory for max available pages count.*/
    unsigned char map_chunks_bit_array[(MAX_MMAP_PAGES_COUNT)/8]; 
    //
    struct BitArray bitarray;
};

struct MemoryManagerPublicInterface* memory_interface_construct( void *heap_ptr, uint32_t heap_size, void *brk );
/*MemoryManager object internally it's a static variable, so we can
  just get instance */
struct MemoryManagerPublicInterface* memory_interface_instance();


#endif //__MEMORY_SYSCALL_HANDLERS_H__
