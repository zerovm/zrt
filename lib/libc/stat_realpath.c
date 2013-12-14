/*
 * stat implementation intended to used by realpath function 
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

int zrt_zcall_stat_realpath(const char* abspathname, struct stat *stat){
    CHECK_EXIT_IF_ZRT_NOT_READY;

    LOG_SYSCALL_START("abspathname=%p, stat=%p", abspathname, stat);
    
    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(abspathname);
    int ret = transpar_mount->stat(abspathname, stat);

    LOG_SHORT_SYSCALL_FINISH(ret, "abspathname=%s", abspathname);
    return ret;
}
