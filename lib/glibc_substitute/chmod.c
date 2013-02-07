/*
 * chmod.c
 * chmod implementation that substitude glibc stub implementation
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
 * it should be linked instead standard chmod, fchmod;
 **************************************************************************/

/*todo: check if syscall chmod is supported by NACL then use it
*instead of this glibc substitution*/
int chmod(const char *path, mode_t mode){
    LOG_SYSCALL_START("path=%s mode=%o(octal)", path, mode);

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    ZRT_LOG(L_SHORT, "path=%s, mode=%u", path, mode );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    mode = apply_umask(mode);
    char* absolute_path = alloc_absolute_path_from_relative(path);
    int ret = transpar_mount->chmod(absolute_path, mode);
    free(absolute_path);
    LOG_SHORT_SYSCALL_FINISH(ret, "path=%s mode=%o(octal)", path, mode);
    return ret;
}

int fchmod(int fd, mode_t mode){
    LOG_SYSCALL_START("fd=%d mode=%o(octal)", fd, mode);
    errno=0;

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);
    /*update mode according to the mask and propogate it to fchmod implementation*/
    mode = apply_umask(mode);
    int ret = transpar_mount->fchmod(fd, mode);
    LOG_SHORT_SYSCALL_FINISH(ret, "fd=%d mode=%o(octal)", fd, mode);
    return ret;
}
