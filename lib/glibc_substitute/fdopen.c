/*
 * fdopen.c
 * fdopen implementation that substitude glibc stub implementation
 *
 *  Created on: 24.12.2012
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
#include "mount_specific_implem.h"
#include "mounts_manager.h"
#include "mounts_interface.h"
#include "path_utils.h"  /*helper*/


/*get pointer to file structure
 *substitude glibc implementation */
FILE *fdopen(int fd, const char *mode){
    LOG_SYSCALL_START("fd=%d mode=%s", fd, mode);
    FILE* f = NULL;

    struct MountsManager* mm = mounts_manager();        /*get access to main mounts object*/
    struct MountsInterface* mif = mm->mount_byhandle(fd); /*get valid mount or NULL*/
    if ( mif ){
	struct mount_specific_implem* implem = mif->implem();
    	assert(implem);                                /*mount specific implem can't be NULL*/
	
	/*check if handle has appropriate path*/
	const char* path = implem->handle_path(fd);
	
	/*run fopen with path parameter instead fd handle*/
	if ( path ){
	    ZRT_LOG(L_SHORT, "fopen(path:%s, mode:%s)", path, mode);
	    f = fopen(path, mode);
	}
    }

    if ( f == NULL ){
	SET_ERRNO(EBADF);
    }

    LOG_SYSCALL_FINISH( (f==NULL), "fd=%d mode=%s", fd, mode );
    return f;
}

