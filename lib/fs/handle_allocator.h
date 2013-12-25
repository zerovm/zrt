/*
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

#ifndef HANDLE_ALLOCATOR_H_
#define HANDLE_ALLOCATOR_H_

#include <sys/stat.h>
#include <stdint.h>

#define MAX_HANDLES_COUNT 1000

struct MountsPublicInterface;

/*interface*/
struct HandleAllocator{
    int (*allocate_handle)(struct MountsPublicInterface* mount_fs);
    int (*allocate_reserved_handle)( struct MountsPublicInterface* mount_fs, int handle );
    int (*free_handle)(int handle);

    struct MountsPublicInterface* (*mount_interface)(int handle);

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


struct HandleAllocator* get_handle_allocator();

#endif /* HANDLE_ALLOCATOR_H_ */
