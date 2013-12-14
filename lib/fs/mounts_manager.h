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

#ifndef MOUNTS_MANAGER_H_
#define MOUNTS_MANAGER_H_

#include <linux/limits.h>

struct MountsInterface;

struct MountInfo{
    char mount_path[PATH_MAX]; /*for example "/", "/dev" */
    struct MountsInterface* mount;
};


struct MountsManager{
    int (*mount_add)( const char* path, struct MountsInterface* filesystem_mount );
    int (*mount_remove)( const char* path );

    struct MountInfo* (*mountinfo_bypath)(const char* path);
    struct MountsInterface* (*mount_bypath)( const char* path );
    struct MountsInterface* (*mount_byhandle)( int handle );

    const char* (*convert_path_to_mount)(const char* full_path);

    struct HandleAllocator* handle_allocator;
};


struct MountsManager* get_mounts_manager();
struct MountsManager* mounts_manager(); /*accessor*/


#endif /* MOUNTS_MANAGER_H_ */
