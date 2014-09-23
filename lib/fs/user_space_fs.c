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

#include "user_space_fs.h"
#include "mounts_manager.h"

int mount_user_fs(struct MountsPublicInterface* fs, const char *mountpoint){
    struct MountsManager *man = mounts_manager();
    assert(man!=NULL);
    return man->mount_add(mountpoint, fs);
}


