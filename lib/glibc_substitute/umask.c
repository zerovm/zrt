/*
 * umask.c
 * umask implementation that substitude glibc stub implementation
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

/*************************************************************************
 * glibc substitution. Implemented functions below should be linked
 * instead of standard syscall that not implemented by NACL glibc
 * it should be linked instead standard umask;
 **************************************************************************/

/*sets umask ang et previous value*/
mode_t umask(mode_t mask){
    LOG_SYSCALL_START(NULL,0);
    /*save new umask and return prev*/
    mode_t prev_umask = get_umask();
    char umask_str[11];
    sprintf( umask_str, "%o", mask );
    setenv( UMASK_ENV, umask_str, 1 );
    ZRT_LOG(L_SHORT, "%s", umask_str);
    LOG_SYSCALL_FINISH(0);
    return prev_umask;
}
