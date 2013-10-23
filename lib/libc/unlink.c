/*
 * unlink.c
 * unlink implementation
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

#include "zcalls.h"
#include "zcalls_zrt.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "zrt_check.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "utils.h"

/*************************************************************************
 * Implementation used by glibc, through zcall interface; It's not using weak alias;
 **************************************************************************/

int zrt_zcall_unlink(const char *pathname){
    CHECK_EXIT_IF_ZRT_NOT_READY;

    LOG_SYSCALL_START("pathname=%s", pathname);

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    char temp_path[PATH_MAX];
    char* absolute_path = zrealpath(pathname, temp_path);
    int ret = transpar_mount->unlink(absolute_path);
    LOG_SHORT_SYSCALL_FINISH(ret, "pathname=%s", pathname);
    return ret;
}
