/*
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


#ifndef CHANNELS_MOUNT_H_
#define CHANNELS_MOUNT_H_

#include <fcntl.h>
#include "mounts_interface.h" //struct MountsPublicInterface

#include "zrt_defines.h" //CONSTRUCT_L

/*name of constructor*/
#define CHANNELS_FILESYSTEM channels_filesystem_construct 
#define CHANNEL_MODE_UPDATER channel_mode_updater_construct

#define FIRST_NON_RESERVED_INODE 11
#define INODE_FROM_ZVM_INODE(zvm_inode) (FIRST_NON_RESERVED_INODE+zvm_inode)
#define ZVM_INODE_FROM_INODE(inode) (inode-FIRST_NON_RESERVED_INODE)

struct ZVMChannel;
struct HandleAllocator;

/*used by mapping nvram section for setting custom channel type*/
struct ChannelsModeUpdaterPublicInterface{
    /*used by mapping nvram section for setting custom channel type*/
    void (*set_channel_mode)(struct ChannelsModeUpdaterPublicInterface* this_, 
			     const char* channel_name, uint mode);
};

/*@param mode_updater Create object and set provided pointer*/
struct MountsPublicInterface* 
channels_filesystem_construct ( struct ChannelsModeUpdaterPublicInterface** mode_updater,
			        struct HandleAllocator* handle_allocator,
				const struct ZVMChannel* zvm_channels, int zvm_channels_count,
				const struct ZVMChannel* emu_channels, int emu_channels_count);

struct ChannelsModeUpdaterPublicInterface*
channel_mode_updater_construct(struct MountsPublicInterface* channels_mount);

#endif /* CHANNELS_MOUNT_H_ */
