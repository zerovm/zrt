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
#include "dyn_array.h" /*struct DynArray*/

struct fuse_operations;
struct MountsPublicInterface;
struct OpenFilesPool;

enum { 
    EAbsolutePathNotExpected = 0,
    EAbsolutePathExpected
};

enum {
    EFuseProxyModeEnabled = 0,
    EFuseProxyModeDisabled
};

struct MountInfo{
    char *mount_path; /*for example "/", "/dev" */
    char expect_absolute_path;
    struct MountsPublicInterface* mount;
};


struct MountsManager{
    /*Add to dynamic list of mounts the new one, caller 
      should not destroy filesystem_mount until exit;
      @param expect_absolute_path 1-path will be passed  into
      fs as is, without transformation, 0 will cut first part of path 
      which is actually is mount path;
      *@return 0 if OK, on error it returns -1 and set errno:
      *ENOTEMPTY - no empty slots to add mount; 
      *EBUSY - mount with specified mountpoint already exist*/
    int (*mount_add)( struct MountsManager *mounts_manager, 
                      const char* path, 
                      struct MountsPublicInterface* filesystem_mount,
                      char expect_absolute_path);

    /*fuse support, the same as mount_add
      @param expect_absolute_path 1-path will be passed  into
      fs as is, without transformation, 0 will cut first part of path 
      which is actually is mount path;
      @param proxy_mode Set to EFuseProxyModeEnabled if mount is implements 
      own file system calls by using standard file system function, otherwise 
      set to EFuseProxyModeDisabled; This is only actual for fuse.
    */
    int (*fusemount_add)( struct MountsManager *mounts_manager,
                          const char* path, 
                          struct fuse_operations* fuse_mount,
                          char expect_absolute_path,
                          char proxy_mode);
    int (*mount_remove)( struct MountsManager *mounts_manager,
                         const char* path );

    struct MountInfo* (*mountinfo_bypath)(struct MountsManager *mounts_manager,
                                          const char* path, int *mount_index);
    struct MountsPublicInterface* (*mount_bypath)( struct MountsManager *mounts_manager,
                                                   const char* path );
    struct MountsPublicInterface* (*mount_byhandle)( struct MountsManager *mounts_manager,
                                                     int handle );

    const char* (*convert_path_to_mount)(struct MountsManager *mounts_manager,
                                         const char* full_path);
    /*data*/
    struct DynArray mount_items; /*array for struct MountInfo*/
};

struct MountsManager* alloc_mounts_manager();


#endif /* MOUNTS_MANAGER_H_ */
