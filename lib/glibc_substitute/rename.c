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

#include "zrtsyscalls.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "path_utils.h"


/*************************************************************************
 * glibc substitution. Implemented functions below should be linked
 * instead of standard syscall that not implemented by NACL glibc
 * it should be linked instead standard rename;
 **************************************************************************/

int rename(const char *oldpath, const char *newpath){
    LOG_SYSCALL_START(NULL,0);

    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    int ret;
    errno=0;
    ZRT_LOG_PARAM(L_SHORT, P_TEXT, oldpath);
    ZRT_LOG_PARAM(L_SHORT, P_TEXT, newpath);
    struct stat oldstat;
    ret = stat(oldpath, &oldstat );
    if ( !ret ){
	ZRT_LOG(L_SHORT, "oldpath ok %d",1);
	struct stat newstat;
	char* absolute_path = alloc_absolute_path_from_relative(newpath);
	ret = transpar_mount->stat(absolute_path, &newstat);
	free(absolute_path);
	if ( ret == 0 || (ret != 0 && errno == ENOENT) ){
	    ZRT_LOG(L_SHORT, "newpath ok %d",1);
	    /*if oldpath exist and new filename does not exist, then
	     *read old file contents into buffer then create and write new file
	     *contents, close files, and remove old file from FS
	     */
	    char* oldbuf = malloc(oldstat.st_size);
	    int oldfd = open(oldpath, O_RDONLY);
	    int bytes = read(oldfd, oldbuf, oldstat.st_size);
	    close(oldfd);
	    ZRT_LOG(L_EXTRA, "bytes=%d, st_size=%d", bytes, (int)oldstat.st_size);
	    assert(bytes==oldstat.st_size);
	    /*if new file path exist then remove it*/
	    if ( ret == 0 ){
		remove(newpath);
	    }
	    /*create new file*/
	    int newfd = open(newpath, O_CREAT | O_WRONLY);
	    if ( newfd >=0 ){
		int bytes_w = write(newfd, oldbuf, oldstat.st_size);
		close(newfd);
		ZRT_LOG(L_EXTRA, "bytes_w=%d, st_size=%d", bytes, (int)oldstat.st_size);
		assert(bytes_w==oldstat.st_size);
		remove(oldpath);
		ret=0; /*rename success*/
	    }
	    free(oldbuf);
	}
    }

    LOG_SYSCALL_FINISH(ret);
    return ret;
}
