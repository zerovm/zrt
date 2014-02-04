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


#ifndef CHANNEL_ARRAY_ITEM_H_
#define CHANNEL_ARRAY_ITEM_H_

#include "channel_array_item.h" //struct ChannelArrayItem
#include "zvm.h" // struct ZVMChannel;
#include "fcntl.h" //struct flock

/*Channel info we need to keep at runtime(For opened channels)
 *suitable data is: opened flags, mode, i/o positions*/
struct ZrtChannelRt{
    int     inode;
    /*For currently opened file flags, refer to handle_alocator*/
    int64_t sequential_access_pos; /*sequential read, sequential write*/
    int64_t random_access_pos;     /*random read, random write*/
    int64_t maxsize;               /*synthethic size. based on maximum position of cursor pos 
				     channel for all I/O requests*/
    int     mode;                  /*channel type, taken from mapping nvram section*/
    int     emu;                   /*equal to 1 if it's emulated channel (not provided by zerovm)*/
    struct flock fcntl_flock;      /*lock flag for support fcntl locking function*/
};


struct ChannelArrayItem{
    const struct ZVMChannel*  channel;
    struct ZrtChannelRt channel_runtime;
};



#endif /* CHANNEL_ARRAY_ITEM_H_ */
