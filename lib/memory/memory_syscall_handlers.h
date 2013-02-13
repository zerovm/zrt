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

/* Low level memory management functions; syscalls should be translated
 * to appropriate object functions;
 * Note that all member functions has 'MemoryInterface* this' param, it's value should
 * be actual for every call, because this used as 'this' pointer and needed to get 
 * access to data that object holdes, also it's makes available to call another
 * object functions;*/
struct MemoryInterface{
    /**/
    void (*init)(struct MemoryInterface* this, void *heap_ptr, uint32_t heap_size);
    int (*sysbrk)(struct MemoryInterface* this, void *addr);
    
    /* @param addr ignored
     * @param length length of the mapping, it's used when fd vale is not real
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
    
    /* @param addr should be actual address starting map region
     * @param length ignored  
     * unmap memory range, in practice memory will released at specified addr*/
    int (*munmap)(struct MemoryInterface* this, void *addr, size_t length);

    //data
    void* heap_ptr;
    uint32_t heap_size;
    void* brk;
};

struct MemoryInterface* memory_interface( void *heap_ptr, uint32_t heap_size );

#endif //__MEMORY_SYSCALL_HANDLERS_H__
