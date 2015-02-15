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

#ifndef __FUSE_INTERFACE_H__
#define __FUSE_INTERFACE_H__

#include <sys/uio.h> /*struct uiovec*/
#include <stdint.h>

struct stat;
struct statvfs;
struct flock;
struct utimbuf;

typedef int (*fs_main)(int, char**);

#ifndef FUSEGLUE_EXT
struct fuse_operations {
    /*stub*/
};
#endif

/*ZRT API*/
fs_main
fusefs_entrypoint_get_args_by_fsname(const char *fsname, int write_mode, 
                                     const char *mntfile, const char *mntpoint,
				     int *argc, char ***argv);

/*Run userfs prefix_main() as part of zrt infrastructure.*/
int fuse_main_wrapper(const char *mount_point, 
                      int (*fs_main)(int, char**), int fs_argc, char **fs_argv);
/*Initiate mount close, actually it releases cond lock in fuse_main*/
void mount_exit();


#endif //__FUSE_INTERFACE_H__

