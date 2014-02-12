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

#include "open_file_description.h" //const struct OpenFileDescription

#define MAX_HANDLES_COUNT 1000

struct MountsPublicInterface;

struct HandleItem{
    ino_t inode;
    int   open_file_description_id;
    struct MountsPublicInterface* mount_fs;
};


/*interface*/
struct HandleAllocator{
    /**/
    int (*allocate_handle)(struct MountsPublicInterface* mount_fs, ino_t inode, int open_file_desc_id);
    int (*allocate_handle2)(struct MountsPublicInterface* mount_fs, ino_t inode, int open_file_desc_id, int handle);
    int (*free_handle)(int handle);

    /*Check if handle is related to the specified fs
     *@return 0 if matched, -1 if not*/
    int (*check_handle_is_related_to_filesystem)(int handle, struct MountsPublicInterface* fs);
    struct MountsPublicInterface* (*mount_interface)(int handle);

    const struct HandleItem* (*entry)(int handle);
    const struct OpenFileDescription* (*ofd)(int handle);
};


struct HandleAllocator* get_handle_allocator();

#endif /* HANDLE_ALLOCATOR_H_ */
