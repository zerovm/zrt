/*
 * truncate.c
 * truncate, ftruncate
 * implementation that substitude glibc stub implementation
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
#include "mount_specific_implem.h"
#include "mounts_manager.h"
#include "mounts_interface.h"
#include "path_utils.h"

#define LSEEK_SET_ASSERT_IF_FAIL(fd, length){			\
	/*set file size and test correctness of lseek result*/	\
	off_t new_pos = lseek( fd, (off_t)length, SEEK_SET);	\
	assert(new_pos==(off_t)length);				\
    }

#define CHECK_NON_NEGATIVE_VALUE_RETURN_ERROR(check_val)	\
    if ( check_val < 0 ) return check_val; /*error return*/

/*write into file length null bytes starting from current pos*/
static int write_file_padding(int fd, off_t length){
    #define BUFSIZE 0x10000
    char buffer[BUFSIZE];
    memset(buffer, '\0', BUFSIZE);
    ssize_t wrote;
    off_t remain_bytes=length;
    do{
	wrote=write(fd, buffer, remain_bytes>BUFSIZE?BUFSIZE:remain_bytes);
	remain_bytes = wrote>0? remain_bytes-wrote: wrote;
    }while(remain_bytes>0);
    /*remain_bytes holds -1 if error occured, or 0 if all correct*/
    return remain_bytes;
}


/*************************************************************************
 * glibc substitution. Implemented functions below should be linked
 * instead of standard syscall that not implemented by NACL glibc
 **************************************************************************/

int ftruncate(int fd, off_t length){
    LOG_SYSCALL_START("fd=%d,length=%lld", fd, length);
    
    struct MountsInterface* transpar_mount = transparent_mount();
    assert(transpar_mount);

    errno=0;
    off_t saved_pos;
    off_t fsize;
    /*save file position*/
    saved_pos = lseek( fd, 0, SEEK_CUR);
    CHECK_NON_NEGATIVE_VALUE_RETURN_ERROR(saved_pos);

    /*get end pos of file*/
    fsize = lseek( fd, 0, SEEK_END);
    CHECK_NON_NEGATIVE_VALUE_RETURN_ERROR(fsize);

    /*if filesize should be increased then just write null bytes '\0' into*/
    if ( length > fsize ){
	/*set cursor to the end of file, and check assertion*/
	off_t endpos = lseek( fd, fsize, SEEK_SET);
	CHECK_NON_NEGATIVE_VALUE_RETURN_ERROR(fsize);
	assert(endpos==fsize);

	/*just write amount of bytes starting from end of file*/
	int res = write_file_padding(fd, length-fsize);
	CHECK_NON_NEGATIVE_VALUE_RETURN_ERROR(res);
    }
    else{ 
	/*truncate data if user wants to reduce filesize
	 *set new file size*/
	int res = transpar_mount->ftruncate_size(fd, length);	
	CHECK_NON_NEGATIVE_VALUE_RETURN_ERROR(res);
    }

    /*restore file position, it's should stay unchanged*/
    if ( saved_pos < length ){
	LSEEK_SET_ASSERT_IF_FAIL(fd, saved_pos);
    }
    else if (saved_pos > length){
	/*it is impossible to restore oldpos because it's not valid anymore
	 *it's pointing beyond of the file*/
    }

    /*final check of real file size and expected size*/
    struct stat st;
    int ret = fstat(fd, &st);
    assert(ret==0);
    ZRT_LOG(L_INFO, "real truncated file size is %lld", st.st_size );
    assert(st.st_size==length);

    LOG_SHORT_SYSCALL_FINISH( 0, "fd=%d, old_length=%lld,new_length=%lld", 
			      fd, fsize, length);
    return 0;
}


int truncate(const char *path, off_t length){
    LOG_SYSCALL_START("path=%s,length=%lld", path, length);

    int ret=-1;
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    char* absolute_path = alloc_absolute_path_from_relative( path );

    struct MountsManager* mm = mounts_manager();        /*get access to main mounts object*/
    struct MountsInterface* mif = mm->mount_bypath(absolute_path); /*get valid mount or NULL*/
    if ( mif ){
	struct mount_specific_implem* implem = mif->implem();
    	assert(implem);                                /*mount specific implem can't be NULL*/

	FILE * f= fopen(absolute_path, "w");
	if (f){
	    /*get file handle from FILE object and call existing ftruncate implementation*/
	    ret = ftruncate(f->_fileno, length);
	    fclose(f);
	}
	else{
	    SET_ERRNO(ENOENT);
	}
    }
    
    free(absolute_path);
    LOG_SHORT_SYSCALL_FINISH( ret, "path=%s,length=%lld", path, length);
    return ret;
}
