/*
 * transparent_mount.h
 *
 *  Created on: 19.09.2012
 *      Author: yaroslav
 */

#ifndef TRANSPARENT_MOUNT_H_
#define TRANSPARENT_MOUNT_H_

struct MountsInterface;
struct MountsManager;

struct MountsInterface* alloc_transparent_mount( struct MountsManager* mounts_manager );

#endif /* TRANSPARENT_MOUNT_H_ */
