/*
 * fcntl.c
 * fcntl implementation that substitude glibc stub implementation
 *
 *  Created on: 19.01.2013
 *      Author: yaroslav
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

#include "zrtsyscalls.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "path_utils.h"
#include "enum_strings.h"


/*************************************************************************
 * glibc substitution. Implemented functions below should be linked
 * instead of standard syscall that not implemented by NACL glibc
 * it should be linked instead standard fcntl;
 **************************************************************************/

/*override system glibc implementation */
int fcntl(int fd, int cmd, ... /* arg */ ){
    int ret=0;
    LOG_SYSCALL_START("fd=%d cmd=%d", fd, cmd);
    errno=0;

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    va_list args;
    va_start(args, cmd);
    if ( cmd == F_SETLK || cmd == F_SETLKW || cmd == F_GETLK ){
	struct flock* input_lock = va_arg(args, struct flock*);
	ZRT_LOG(L_SHORT, "flock=%p", input_lock );
	ret = transpar_mount->fcntl(fd, cmd, input_lock);
    }
    va_end(args);
    LOG_SHORT_SYSCALL_FINISH( ret, "fd=%d, cmd=%s", fd, STR_FCNTL_CMD(cmd));
    return ret;
}

