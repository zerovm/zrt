/*
 * fcntl_implem.h
 * Implementation of fcntl call for any mount type.
 * It is used interface to get access to mount specific implementtions;
 *  Created on: 17.12.2012
 *      Author: yaroslav
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

