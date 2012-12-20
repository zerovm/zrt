/*
 * fcntl_implem.h
 * Stub implementation for fcntl call that intended to use by any zrt mount type.
 * Only single process can operate with a lock, as not supported multiprocess environment.
 * It is used interface to get access to mount specific implementations;
 *  Created on: 17.12.2012
 *      Author: YaroslavLitvinov
 */

#ifndef __FCNTL_IMPLEM_H__
#define __FCNTL_IMPLEM_H__

#include <unistd.h>
#include <fcntl.h>

/*external*/
struct mount_specific_implem;

int fcntl_implem(struct mount_specific_implem* implem, 
		 int fd, int cmd, ...);

#endif //__FCNTL_IMPLEM_H__

