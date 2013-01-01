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
    /*return 0 if handle not valid, or 1 if handle is correct*/
    int  (*check_handle)(int handle);
    /*if wrong handle gt NULL*/
    const char* (*handle_path)(int handle);

    const struct flock* (*flock_data)( int fd );
    int (*set_flock_data)( int fd, const struct flock* flock_data );
};

#endif //__MOUNT_SPECIFIC_IMPLEM_H__

