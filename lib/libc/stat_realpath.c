/*
 * mkdir.c
 * mkdir implementation that substitude glibc stub implementation
 *
 *  Created on: 17.11.2013
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

int zrt_zcall_stat_realpath(const char* abspathname, struct stat *stat){
    CHECK_EXIT_IF_ZRT_NOT_READY;

    LOG_SYSCALL_START("abspathname=%p, stat=%p", abspathname, stat);
    
    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(abspathname);
    int ret = transpar_mount->stat(abspathname, stat);

    LOG_SHORT_SYSCALL_FINISH(ret, "abspathname=%s", abspathname);
    return ret;
}
