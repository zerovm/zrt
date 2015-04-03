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

#ifndef __USER_SPACE_FS_H__
#define __USER_SPACE_FS_H__

#include "mounts_interface.h"

struct fuse_operations;

/*original zrt fs*/
int mount_user_fs(struct MountsPublicInterface* fs, 
                  const char *mountpoint, char expect_absolute_path);

/*fuse fs
  @param proxy_mode - See MountsManager*/
int mount_fuse_fs(struct fuse_operations* fs, 
                  const char *mountpoint, char expect_absolute_path, char proxy_mode);

#endif //__USER_SPACE_FS_H__
