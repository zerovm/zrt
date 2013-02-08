/*
 * unlink.c
 * unlink implementation that substitude glibc stub implementation
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
 * it should be linked instead standard unlink;
 **************************************************************************/

int link(const char *oldpath, const char *newpath){
}


int unlink(const char *pathname){
    LOG_SYSCALL_START("pathname=%s", pathname);

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    ZRT_LOG(L_SHORT, "pathname=%s", pathname );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    char* absolute_path = alloc_absolute_path_from_relative(pathname);
    int ret = transpar_mount->unlink(absolute_path);
    free(absolute_path);
    LOG_SHORT_SYSCALL_FINISH(ret, "pathname=%s", pathname);
    return ret;
}
