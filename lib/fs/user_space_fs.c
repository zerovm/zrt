/*
 * USER SPACE FILESYSTEM API
 *
 * Copyright (c) 2013, LiteStack, Inc.
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

#include <assert.h>

#include "zcalls_zrt.h"
#include "user_space_fs.h"
#include "mounts_manager.h"


int mount_user_fs(struct MountsPublicInterface* fs, const char *mountpoint, 
                  char expect_absolute_path){
    struct MountsManager *man = get_system_mounts_manager();
    assert(man!=NULL);
    return man->mount_add(man, mountpoint, fs, expect_absolute_path);
}

int mount_fuse_fs(struct fuse_operations* fuse_op, const char *mountpoint, 
                  char expect_absolute_path, char proxy_mode){
    struct MountsManager *man = get_system_mounts_manager();
    assert(man!=NULL);
    return man->fusemount_add(man, mountpoint, fuse_op, expect_absolute_path, proxy_mode);
}
