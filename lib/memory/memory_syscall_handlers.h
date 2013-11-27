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

/* Low level memory management functions, here are syscalls
 * implementation.  Note that all member functions has
 * 'MemoryInterface* this' param, and it can't be a NULL, because it
 * used as 'this' pointer and need to access data that object hold,
 * also it's makes available to call another object functions;*/
struct MemoryInterface{
    /*Low level memory allocators initializer.  Whole heap memory
     *already preallocated/mmaped by zerovm, just before untrusted
     *session starts. But we want to be able to use both SBRK and MMAP
     *allocators. So to avoid overlaping of different memory ranges
     *divide heap memory into 2 parts: sbrk range resides on lowest
     *addresses, mmap range - on highest addresses.  
     *Memory range I - SBRK : [sbrk_heap_ptr, mmap_heap_ptr): 
     *sbrk memory ends up where mmap region starts.  
     *Memory range II- MMAP : [mmap_heap_ptr,mmap_heap_ptr+mmap_heap_size] 
     @param sbrk_heap_ptr start of mmap region 
     @param mmap_heap_ptr start of mmap region 
     @param mmap_heap_size size mmap range*/
    void (*init)(struct MemoryInterface* this, void *sbrk_heap_ptr, 
		 void *mmap_heap_ptr, uint32_t mmap_heap_size);
    int (*sysbrk)(struct MemoryInterface* this, void *addr);
    
    /* MMAP emulation in user-space implementation.
     *  @param addr ignored
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
    
    /* MUNMAP emulation in user-space implementation.
     * @param addr should be actual address starting map region
     * @param length ignored  
     * unmap memory range, in practice memory will released at specified addr*/
    int (*munmap)(struct MemoryInterface* this, void *addr, size_t length);

    //data
    void*    heap_sbrk_ptr; /*sbrk memory ends up where mmap region starts*/
    void*    heap_sbrk_brk;
    void*    heap_mmap_ptr_aligned;
    uint32_t heap_mmap_size_aligned;
};

struct MemoryInterface* get_memory_interface( void *heap_ptr, uint32_t heap_size );

#endif //__MEMORY_SYSCALL_HANDLERS_H__
