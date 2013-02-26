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

#include "zcalls_zrt.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "path_utils.h"


/*************************************************************************
 * glibc substitution. Implemented functions below should be linked
 * instead of standard syscall that not implemented by NACL glibc
 **************************************************************************/

int mkdir(const char* pathname, mode_t mode){
    LOG_SYSCALL_START("pathname=%p, mode=%o(octal)", pathname, (uint32_t)mode);
    
    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    char* absolute_path = alloc_absolute_path_from_relative( pathname );
    mode = apply_umask(mode);
    int ret = transpar_mount->mkdir( absolute_path, mode );
    int errno_mkdir = errno; /*save mkdir errno before stat request*/
    /*print stat data of newly created directory*/
    struct stat st;
    int ret2 = transpar_mount->stat(absolute_path, &st);
    if ( ret2 == 0 ){
	debug_mes_stat(&st);
    }
    /**/
    free(absolute_path);
    if ( ret == -1 )
	errno = errno_mkdir;/*restore mkdir errno after stat request completed*/
    else
	errno =0; /*rest errno if OK*/

    LOG_SHORT_SYSCALL_FINISH(ret, "pathname=%s, mode=%o(octal)", pathname, (uint32_t)mode);
    return ret;
}
