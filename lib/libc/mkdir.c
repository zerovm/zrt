/*
 * mkdir implementation that replaces glibc stub implementation
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

#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "zcalls.h"
#include "zcalls_zrt.h"
#include "zrt_check.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "utils.h"
#include "enum_strings.h"

/*************************************************************************
 * Implementation used by glibc, through zcall interface; It's not using weak alias;
 **************************************************************************/

int zrt_zcall_mkdir(const char* pathname, mode_t mode){
    CHECK_EXIT_IF_ZRT_NOT_READY;
    char* absolute_path;
    char temp_path[PATH_MAX];
    int ret=-1;
    errno=0;
    LOG_SYSCALL_START("pathname=%p, mode=%o(octal)", pathname, (uint32_t)mode);
    
    struct MountsPublicInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    if ( (absolute_path = zrealpath(pathname, temp_path)) != NULL ){
	ret = transpar_mount->mkdir(transpar_mount, absolute_path, mode);
    }

    LOG_SHORT_SYSCALL_FINISH(ret, "pathname=%s, mode=%o(octal)", pathname, (uint32_t)mode);
    return ret;
}
