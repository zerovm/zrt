/*
 * mkdir.c
 * mkdir implementation that substitude glibc stub implementation
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

#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "zcalls.h"
#include "zcalls_zrt.h"
#include "zrt_check.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "utils.h"
#include "enum_strings.h"

/*************************************************************************
 * Implementation used by glibc, through zcall interface; It's not using weak alias;
 **************************************************************************/

int zrt_zcall_mkdir(const char* pathname, mode_t mode){
    CHECK_EXIT_IF_ZRT_NOT_READY;
    char* absolute_path;
    char temp_path[PATH_MAX];
    int ret=-1;
    errno=0;
    LOG_SYSCALL_START("pathname=%p, mode=%o(octal)", pathname, (uint32_t)mode);
    
    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    if ( (absolute_path = zrealpath(pathname, temp_path)) != NULL ){
	ret = transpar_mount->mkdir(absolute_path, mode);
    }

    LOG_SHORT_SYSCALL_FINISH(ret, "pathname=%s, mode=%o(octal)", pathname, (uint32_t)mode);
    return ret;
}
