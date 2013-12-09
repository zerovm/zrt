/*
 * getsysstats.c
 * 
 *
 *  Created on: 3.12.2013
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
#include "utils.h"
#include "memory_syscall_handlers.h"

/*************************************************************************
 * Implementation used by glibc, through zcall interface; It's not using weak alias;
 **************************************************************************/

int zrt_zcall_get_phys_pages(){
    CHECK_EXIT_IF_ZRT_NOT_READY;
    errno=0;
    LOG_SYSCALL_START(P_TEXT, "");    

    struct MemoryInterface* memif = memory_interface_instance();
    long int ret = memif->get_phys_pages(memif);
    LOG_SHORT_SYSCALL_FINISH( ret, "get_phys_pages=%ld", ret );
    return ret;
}


int zrt_zcall_get_avphys_pages(){
    CHECK_EXIT_IF_ZRT_NOT_READY;
    errno=0;
    LOG_SYSCALL_START(P_TEXT, "");    

    struct MemoryInterface* memif = memory_interface_instance();
    long int ret = memif->get_phys_pages(memif);
    LOG_SHORT_SYSCALL_FINISH( ret, "get_avphys_pages=%ld", ret);
    return ret;
}
