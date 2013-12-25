/*
 * link implementation that replaces glibc stub implementation
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

#include "zcalls.h"
#include "zcalls_zrt.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "zrt_check.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "utils.h"

/*************************************************************************
 * Implementation used by glibc, through zcall interface; It's not using weak alias;
 **************************************************************************/

int zrt_zcall_link(const char *oldpath, const char *newpath){
    CHECK_EXIT_IF_ZRT_NOT_READY;
    char* absolute_path1;
    char* absolute_path2;
    char temp_path1[PATH_MAX];
    char temp_path2[PATH_MAX];
    int ret=-1;
    LOG_SYSCALL_START("oldpath=%s, newpath=%s", oldpath, newpath);

    struct MountsPublicInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(oldpath);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(newpath);
    if ( (absolute_path1 = realpath(oldpath, temp_path1)) != NULL &&
	 (absolute_path2 = zrealpath(newpath, temp_path2)) != NULL){
	ret = transpar_mount->link(transpar_mount, absolute_path1, absolute_path2);
    }

    LOG_SHORT_SYSCALL_FINISH(ret, "oldpath=%s, newpath=%s", oldpath, newpath);
    return ret;
}

