/*
 * mount_specific_implem.h
 * Interface for functions whose implemetation is different for
 * various mounts: channels and MemMount;
 *  Created on: 17.12.2012
 *      Author: yaroslav
 */

#ifndef __MOUNT_SPECIFIC_IMPLEM_H__
#define __MOUNT_SPECIFIC_IMPLEM_H__

#include <unistd.h>
#include <fcntl.h>


struct mount_specific_implem{
    int  (*check_handle)(int handle);
    const struct flock* (*flock_data)( int fd );
    int (*set_flock_data)( int fd, const struct flock* flock_data );
};

#endif //__MOUNT_SPECIFIC_IMPLEM_H__

