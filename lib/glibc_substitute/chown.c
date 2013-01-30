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

#include "zrtsyscalls.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "path_utils.h"


/*************************************************************************
 * glibc substitution. Implemented functions below should be linked
 * instead of standard syscall that not implemented by NACL glibc
 * it should be linked instead standard lchown, chown, fchown;
 **************************************************************************/


int chown(const char *path, uid_t owner, gid_t group){
    LOG_SYSCALL_START("path=%s owner=%u group=%u", path, owner, group);
    errno=0;

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    char* absolute_path = alloc_absolute_path_from_relative(path);
    int ret = transpar_mount->chown(absolute_path, owner, group);
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret, "path=%s", path);
    return ret;
}

int fchown(int fd, uid_t owner, gid_t group){
    LOG_SYSCALL_START("fd=%d owner=%u group=%u", fd, owner, group);

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    ZRT_LOG(L_SHORT, "fd=%d, owner=%u, group=%u", fd, owner, group );
    int ret = transpar_mount->fchown(fd, owner, group);
    LOG_SYSCALL_FINISH(ret, "fd=%d", fd);
    return ret;
}

int lchown(const char *path, uid_t owner, gid_t group){
    LOG_SYSCALL_START("path=%s owner=%u group=%u", path, owner, group);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    /*do not do transformaton path, it's called in nested chown*/
    int ret =chown(path, owner, group);
    LOG_SYSCALL_FINISH(ret, "path=%s", path);
    return ret;
}
