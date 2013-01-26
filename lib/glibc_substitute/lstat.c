/*
 * lstat.c
 * lstat implementation that substitude glibc stub implementation
 *
 *  Created on: 19.01.2013
 *      Author: yaroslav
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

#include "zrtsyscalls.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "path_utils.h"


/*************************************************************************
 * glibc substitution. Implemented functions below should be linked
 * instead of standard syscall that not implemented by NACL glibc
 * it should be linked instead standard lstat;
 **************************************************************************/

int lstat(const char *path, struct stat *buf){
    LOG_SYSCALL_START(NULL,0);

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    ZRT_LOG(L_SHORT, "path=%s, buf=%p", path, buf);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    char* absolute_path = alloc_absolute_path_from_relative( path );
    int ret = transpar_mount->stat(absolute_path, buf);
    free(absolute_path);
    if ( ret == 0 ){
        debug_mes_stat(buf);
    }
    LOG_SYSCALL_FINISH(ret);
    return ret;
}
