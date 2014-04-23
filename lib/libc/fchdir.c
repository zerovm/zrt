/*
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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "zcalls_zrt.h"
#include "zcalls.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "zrt_check.h"
#include "utils.h"
#include "handle_allocator.h"
#include "mounts_interface.h"
#include "mount_specific_interface.h"


/*************************************************************************
 * Implementation used by glibc, through zcall interface; It's not using weak alias;
 **************************************************************************/

int zrt_zcall_fchdir(int fd){
    CHECK_EXIT_IF_ZRT_NOT_READY;
    errno=0;
    LOG_SYSCALL_START(P_TEXT, "");    
    int ret = -1;

    const struct HandleItem* entry = get_handle_allocator()->entry(fd);
    if ( entry == NULL ){
	SET_ERRNO(EBADF);
	return -1;
    }

    /*get reference to filesystem that related to path*/
    struct MountsPublicInterface* fs_implementation = entry->mount_fs;
    struct MountSpecificPublicInterface* fs_specific_implementation 
	= fs_implementation->implem(fs_implementation);

    const char* path = fs_specific_implementation->handle_path(fs_specific_implementation, fd);
    if ( path != NULL ){
	ret = chdir(path);
    }
    else{
	SET_ERRNO(ENOENT);
	return -1;
    }

    LOG_SHORT_SYSCALL_FINISH( ret, "get_phys_pages=%d", ret );
    return ret;
}

