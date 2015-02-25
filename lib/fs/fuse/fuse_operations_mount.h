/*
 *
 * Copyright (c) 2015, Rackspace, Inc.
 * Author: Yaroslav Litvinov, yaroslav.litvinov@rackspace.com
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


#ifndef __FUSE_OPERATIONS_MOUNT_H__
#define __FUSE_OPERATIONS_MOUNT_H__

#include <fs/mounts_interface.h> //struct MountsPublicInterface

#include "zrt_defines.h" //CONSTRUCT_L

/*name of constructor*/
#define FUSE_OPERATIONS_MOUNT fuse_operations_mount_construct

struct HandleAllocator;
struct OpenFilesPool;
struct fuse_operations;

struct MountsPublicInterface* 
fuse_operations_mount_construct( struct HandleAllocator* handle_allocator,
				 struct OpenFilesPool* open_files_pool,
				 struct fuse_operations* fuse_operations,
                                 char proxying_fs_calls);

#endif /* __FUSE_OPERATIONS_MOUNT_H__ */
