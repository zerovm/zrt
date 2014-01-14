/*
 * fcntl_implem.c
 * Stub implementation for fcntl call that intended to use by any zrt mount type.
 * Only single process can operate with a lock, as not supported multiprocess environment.
 * It is used interface to get access to mount specific implementations;
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "mount_specific_interface.h"
#include "fcntl_implem.h"
#include "enum_strings.h"


int fcntl_implem(struct MountSpecificPublicInterface* this, int fd, int cmd, ...){
    int rc=0;
    /* check fd */
    if( this->check_handle(this, fd) == 0 ){
        SET_ERRNO( EBADF );
        return -1;
    }

    switch( cmd ){
    case F_GETFD:
	//rc = zrt_channel->fcntl_flags;
	//ZRT_LOG(L_SHORT, "get fcntl_flags=%d", zrt_channel->fcntl_flags);
	SET_ERRNO(ENOSYS);
	rc=-1;
	break;
    case F_SETFD:{
	va_list args;
	va_start(args, cmd);
	int new_flags = va_arg(args, int);
	va_end(args);
	(void)new_flags;
	//ZRT_LOG(L_SHORT, "fcntl_flags=%d", new_flags);
	//zrt_channel->fcntl_flags = new_flags;
	SET_ERRNO(ENOSYS);
	rc=-1;
	break;
    }
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK:{
	/* implementation issues:
	 * it's not real implementation, lock is always can be set by current process
	 * it's always get F_UNLCK lock type;
	 * it's always allow to set lock type, and return l_type F_UNLCK.*/

	/*get input argument flock structure*/
	va_list args;
	va_start(args, cmd);
	struct flock* input_lock = va_arg(args, struct flock*);
	va_end(args);
	if ( !input_lock ) {
	    ZRT_LOG(L_ERROR, P_TEXT, "flock structure pointer is NULL");
	    SET_ERRNO(EINVAL);
	    rc = -1;
	}
	else{
	    /*flock struct data acquired, log struct fields*/
	    ZRT_LOG(L_SHORT, "argument flock l_type=%s, p=%p", 
		    STR_LOCK_TYPE_FLAGS(input_lock->l_type), input_lock);
	    ZRT_LOG(L_INFO, "argument flock l_whence=%d, l_start=%lld, l_len=%lld", 
		    input_lock->l_whence,
		    input_lock->l_start,
		    input_lock->l_len);

	    input_lock->l_type = F_UNLCK;                   /*always allow set lock/unlock */
	    ZRT_LOG(L_SHORT, "get lock type=%s", STR_LOCK_TYPE_FLAGS(input_lock->l_type));	
	}
	break;
    }
    case F_GETFL:
	/*get file status flags, like flags specified with open function*/
	rc = this->file_status_flags(this, fd);
	break;	
    case F_SETFL:{
	/*set file status flags, like flags specified with open function*/
	/*get input argument flags*/
	va_list args;
	va_start(args, cmd);
	long flags = va_arg(args, long);
	rc = this->set_file_status_flags(this, fd, flags);
	break;	
    }
    default:
	SET_ERRNO(ENOSYS);
	break;
    }
    
    return rc;
}

