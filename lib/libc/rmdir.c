/*
 * rmdir.c
 * rmdir implementation that substitude glibc stub implementation
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

int zrt_zcall_rmdir(const char *pathname){
    if ( !is_user_main_running() ){
	SAFE_LOG(__func__);
	/*while not initialized completely*/
	errno=ENOSYS;
	return -1;
    }

    LOG_SYSCALL_START("pathname=%s", pathname);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    errno=0;

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);
    char* absolute_path = alloc_absolute_path_from_relative( pathname );
    int ret = transpar_mount->rmdir( absolute_path );
    free(absolute_path);
    LOG_SHORT_SYSCALL_FINISH(ret, "pathname=%s", pathname);
    return ret;
}
