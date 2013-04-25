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
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "path_utils.h"

/*************************************************************************
 * Implementation used by glibc, through zcall interface; It's not using weak alias;
 **************************************************************************/

int zrt_zcall_link(const char *oldpath, const char *newpath){
    if ( !is_zrt_ready() ){
	ZRT_LOG(L_SHORT, "%s %s", __func__, "can't be used while prolog running");
	/*while not initialized completely*/
	errno=ENOSYS;
	return -1;
    }

    LOG_SYSCALL_START("oldpath=%s, newpath=%s", oldpath, newpath);

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(oldpath);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(newpath);
    char* absolute_path1 = alloc_absolute_path_from_relative(oldpath);
    char* absolute_path2 = alloc_absolute_path_from_relative(newpath);

    int ret = transpar_mount->link(absolute_path1, absolute_path2);
    free(absolute_path1);
    free(absolute_path2);
    LOG_SHORT_SYSCALL_FINISH(ret, "oldpath=%s, newpath=%s", oldpath, newpath);
    return ret;
}

