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
#include <stdarg.h>
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
#include "enum_strings.h"


const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

/*************************************************************************
 * Implementation used by glibc, through zcall interface; It's not using weak alias;
 **************************************************************************/

int zrt_zcall_fcntl(int fd, int cmd, ... /* arg */ ){
    CHECK_EXIT_IF_ZRT_NOT_READY;

    int ret=0;
    LOG_SYSCALL_START("fd=%d cmd=%d", fd, cmd);
    errno=0;

    struct MountsPublicInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    va_list args;
    va_start(args, cmd);
    if ( cmd == F_SETLK || cmd == F_SETLKW || cmd == F_GETLK ){
	struct flock* input_lock = va_arg(args, struct flock*);
	ZRT_LOG(L_SHORT, "flock=%p", input_lock );
	ret = transpar_mount->fcntl(transpar_mount, fd, cmd, input_lock);
    }
    else if ( cmd == F_GETFL ){
	ret = transpar_mount->fcntl(transpar_mount, fd, cmd);
	ZRT_LOG(L_INFO, "flags    =%s", byte_to_binary(ret) );
	ZRT_LOG(L_INFO, "O_ACCMODE=%s", byte_to_binary(O_ACCMODE) );
	ZRT_LOG(L_INFO, "O_RDONLY =%s", byte_to_binary(O_RDONLY) );
	ZRT_LOG(L_INFO, "O_WRONLY =%s", byte_to_binary(O_WRONLY) );
	ZRT_LOG(L_INFO, "O_RDWR   =%s", byte_to_binary(O_RDWR) );
	fflush(0);
    }

    va_end(args);
    LOG_SHORT_SYSCALL_FINISH( ret, "fd=%d, cmd=%s", fd, STR_FCNTL_CMD(cmd));
    return ret;
}

