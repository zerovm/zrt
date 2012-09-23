/*
 * handle_allocator.h
 *
 *  Created on: 20.09.2012
 *      Author: yaroslav
 */

#ifndef HANDLE_ALLOCATOR_H_
#define HANDLE_ALLOCATOR_H_

#include <sys/stat.h>
#include <stdint.h>

#define MAX_HANDLES_COUNT 1000

struct MountsInterface;

/*interface*/
struct HandleAllocator{
    int (*allocate_handle)(struct MountsInterface* mount_fs);
    int (*allocate_reserved_handle)( struct MountsInterface* mount_fs, int handle );
    int (*free_handle)(int handle);

    struct MountsInterface* (*mount_interface)(int handle);

    /* get inode
     * @return errcode, 0 ok, -1 not found*/
    int (*get_inode)(int handle, ino_t* inode );
    /* set inode
     * @return errcode, 0 ok, -1 not found*/
    int (*set_inode)(int handle, ino_t inode );

    /* get offset
     * @return errcode, 0 ok, -1 not found*/
    int (*get_offset)(int handle, off_t* offset );
    /* set offset
     * @return errcode, 0 ok, -1 not found*/
    int (*set_offset)(int handle, off_t offset );
};


struct HandleAllocator* alloc_handle_allocator();

#endif /* HANDLE_ALLOCATOR_H_ */
