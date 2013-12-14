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
#include "mounts_interface.h"

#define FIRST_NON_RESERVED_INODE 11
#define INODE_FROM_HANDLE(handle) (FIRST_NON_RESERVED_INODE+handle)

struct ZVMChannel;
struct HandleAllocator;

struct MountsInterface* alloc_channels_mount( struct HandleAllocator* handle_allocator,
        const struct ZVMChannel* channels, int channels_count );

/*used by mapping nvram section for setting custom channel type*/
uint* channel_mode(const char* channel_name);

#endif /* CHANNELS_MOUNT_H_ */
