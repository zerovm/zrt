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

struct fuse_operations;
struct MountsPublicInterface;
struct OpenFilesPool;

struct MountInfo{
    char mount_path[PATH_MAX]; /*for example "/", "/dev" */
    struct MountsPublicInterface* mount;
};


struct MountsManager{
    /*Add to list of mounts the new one, caller should not destroy
      filesystem_mount upon delete; Slots count is limited by
      EMountsCount.  
      *@return 0 if OK, on error it returns -1 and set errno:
      *ENOTEMPTY - no empty slots to add mount; 
      *EBUSY - mount with specified mountpoint already exist*/
    int (*mount_add)( const char* path, struct MountsPublicInterface* filesystem_mount );
    /*fuse support, the same as mount_add*/
    int (*fusemount_add)( const char* path, struct fuse_operations* fuse_mount );
    int (*mount_remove)( const char* path );

    struct MountInfo* (*mountinfo_bypath)(const char* path);
    struct MountsPublicInterface* (*mount_bypath)( const char* path );
    struct MountsPublicInterface* (*mount_byhandle)( int handle );

    const char* (*convert_path_to_mount)(const char* full_path);

    struct HandleAllocator* handle_allocator;
    struct OpenFilesPool* open_files_pool;
};


struct MountsManager* get_mounts_manager();
struct MountsManager* mounts_manager(); /*accessor*/


#endif /* MOUNTS_MANAGER_H_ */
