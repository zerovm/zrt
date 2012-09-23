/*
 * channels_mount.h
 *
 *  Created on: 19.09.2012
 *      Author: yaroslav
 */

#ifndef CHANNELS_MOUNT_H_
#define CHANNELS_MOUNT_H_

#include "mounts_interface.h"

#define CHECK_FLAG(flags, flag) ( (flags & flag) == flag? 1 : 0)
#define SET_ERRNO(err) {errno=err;zrt_log("errno=%d", err);}
#define FIRST_NON_RESERVED_INODE 11
#define INODE_FROM_HANDLE(handle) (FIRST_NON_RESERVED_INODE+handle)

struct ZVMChannel;
struct HandleAllocator;

struct MountsInterface* alloc_channels_mount( struct HandleAllocator* handle_allocator,
        const struct ZVMChannel* channels, int channels_count );

#endif /* CHANNELS_MOUNT_H_ */
