/*
 * chmod.c
 * chmod implementation 
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

#include "zrt.h"
#include "zcalls_zrt.h"
#include "zcalls.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "zrt_check.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "path_utils.h"

/*************************************************************************
 * Implementation used by glibc, through zcall interface; It's not using weak alias;
 **************************************************************************/

int zrt_zcall_chmod(const char *path, mode_t mode){
    CHECK_EXIT_IF_ZRT_NOT_READY;

    LOG_SYSCALL_START("path=%s mode=%o(octal)", path, mode);

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    ZRT_LOG(L_SHORT, "path=%s, mode=%u", path, mode );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    APPLY_UMASK(&mode);
    char* absolute_path = alloc_absolute_path_from_relative(path);
    int ret = transpar_mount->chmod(absolute_path, mode);
    free(absolute_path);
    LOG_SHORT_SYSCALL_FINISH(ret, "path=%s mode=%o(octal)", path, mode);
    return ret;
}

int zrt_zcall_fchmod(int fd, mode_t mode){
    CHECK_EXIT_IF_ZRT_NOT_READY;

    LOG_SYSCALL_START("fd=%d mode=%o(octal)", fd, mode);
    errno=0;

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);
    /*update mode according to the mask and propogate it to fchmod implementation*/
    APPLY_UMASK(&mode);
    int ret = transpar_mount->fchmod(fd, mode);
    LOG_SHORT_SYSCALL_FINISH(ret, "fd=%d mode=%o(octal)", fd, mode);
    return ret;
}
