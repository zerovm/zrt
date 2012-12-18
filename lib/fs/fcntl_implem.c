/*
 * fcntl_implem.c
 * Implementation of fcntl call for any mount type.
 * It is used interface to get access to mount specific implementtions;
 *  Created on: 17.12.2012
 *      Author: yaroslav
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
#include "mount_specific_implem.h"
#include "fcntl_implem.h"
#include "enum_strings.h"


int fcntl_implem(struct mount_specific_implem* implem, int fd, int cmd, ...){
    int rc=0;
    /* check fd */
    if( implem->check_handle(fd) == 0 ){
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
	va_list args;
	va_start(args, cmd);
	struct flock* lock_data = va_arg(args, struct flock*);
	va_end(args);
	if ( !lock_data ) {
	    ZRT_LOG(L_ERROR, P_TEXT, "flock structure pointer is NULL");
	    SET_ERRNO(EINVAL);
	    rc = -1;
	}
	else{
	    /*flock struct data, log another fields*/
	    ZRT_LOG(L_INFO, "argument flock l_type=%s, l_whence=%d, l_start=%lld, l_len=%lld", 
		    LOCK_TYPE_FLAGS(lock_data->l_type),
		    lock_data->l_whence,
		    lock_data->l_start,
		    lock_data->l_len);

	    /*get lock*/
	    if ( cmd == F_GETLK ){
		const struct flock* data = implem->flock_data(fd);
		/*get current lock flag via flock struct*/
		if ( data ){
		    memcpy(lock_data, data, sizeof(struct flock));
		}
		/*get lock type from runtime channel info*/
		ZRT_LOG(L_INFO, "get lock type=%s", LOCK_TYPE_FLAGS(data->l_type));
	    }
	    /*set lock*/
	    else if ( cmd == F_SETLK || cmd == F_SETLKW ){
		/*save lock type in runtime channel info*/
		ZRT_LOG(L_INFO, "set lock type=%s", LOCK_TYPE_FLAGS(lock_data->l_type));
		implem->set_flock_data(fd, lock_data);
	    }
	    else{
		assert(0);
	    }
	}
	break;
	default:
	    SET_ERRNO(ENOSYS);
	    break;
    }
    }

    
    return rc;
}
