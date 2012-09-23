/*
 * mem_mount.h
 *
 *  Created on: 20.09.2012
 *      Author: yaroslav
 */

#ifndef MEM_MOUNT_WRAPER_H_
#define MEM_MOUNT_WRAPER_H_

#include "mounts_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

struct HandleAllocator;

/*todo: nlink support*/

struct MountsInterface* alloc_mem_mount( struct HandleAllocator* handle_allocator );

#ifdef __cplusplus
}
#endif


#endif /* MEM_MOUNT_WRAPER_H_ */
