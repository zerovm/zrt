/*
 * link.c
 * link implementation
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

int zrt_zcall_link(const char *oldpath, const char *newpath){
    CHECK_EXIT_IF_ZRT_NOT_READY;
    char* absolute_path1;
    char* absolute_path2;
    char temp_path1[PATH_MAX];
    char temp_path2[PATH_MAX];
    int ret=-1;
    LOG_SYSCALL_START("oldpath=%s, newpath=%s", oldpath, newpath);

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(oldpath);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(newpath);
    if ( (absolute_path1 = realpath(oldpath, temp_path1)) != NULL &&
	 (absolute_path2 = zrealpath(newpath, temp_path2)) != NULL){
	ret = transpar_mount->link(absolute_path1, absolute_path2);
    }

    LOG_SHORT_SYSCALL_FINISH(ret, "oldpath=%s, newpath=%s", oldpath, newpath);
    return ret;
}

