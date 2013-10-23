/*
 * chown.c
 * chown implementation that substitude glibc stub implementation
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

#include "zcalls_zrt.h"
#include "zcalls.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "zrt_check.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "utils.h"

/*************************************************************************
 * Implementation used by glibc, through zcall interface; It's not using weak alias;
 **************************************************************************/

int zrt_zcall_chown(const char *path, uid_t owner, gid_t group){
    CHECK_EXIT_IF_ZRT_NOT_READY;

    LOG_SYSCALL_START("path=%s owner=%u group=%u", path, owner, group);
    errno=0;

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    char temp_path[PATH_MAX];
    char* absolute_path = zrealpath(path, temp_path);
    int ret = transpar_mount->chown(absolute_path, owner, group);
    LOG_SHORT_SYSCALL_FINISH( ret, "path=%s", path);
    return ret;
}

int zrt_zcall_fchown(int fd, uid_t owner, gid_t group){
    CHECK_EXIT_IF_ZRT_NOT_READY;

    LOG_SYSCALL_START("fd=%d owner=%u group=%u", fd, owner, group);

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    ZRT_LOG(L_SHORT, "fd=%d, owner=%u, group=%u", fd, owner, group );
    int ret = transpar_mount->fchown(fd, owner, group);
    LOG_SHORT_SYSCALL_FINISH( ret, "fd=%d", fd);
    return ret;
}
