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


#ifndef CHANNELS_ARRAY_H_
#define CHANNELS_ARRAY_H_

#include "channel_array_item.h" //struct ChannelArrayItem
#include "zrt_defines.h" //CONSTRUCT_L

/*name of constructor*/
#define CHANNELS_ARRAY channels_array_construct 


struct ChannelsArrayPublicInterface{
    int (*count)(struct ChannelsArrayPublicInterface* this);
    /*@return NULL if not found, or item pointer if fetched*/
    struct ChannelArrayItem* (*get)(struct ChannelsArrayPublicInterface* this, int index);
    /*@param index return pointer to matched item
      @return NULL if not found, or item pointer is matched*/
    struct ChannelArrayItem* (*match_by_name)(struct ChannelsArrayPublicInterface* this, 
					      const char* channel_name);
    struct ChannelArrayItem* (*match_by_inode)(struct ChannelsArrayPublicInterface* this, 
					      int inode);
};


/*@return result pointer can be casted to struct BitArray*/
struct ChannelsArrayPublicInterface* 
channels_array_construct(const struct ZVMChannel* zvm_channels, 
			 int zvm_channels_count,
			 const struct ZVMChannel* emu_channels,
			 int emu_channels_count);



#endif /* CHANNELS_ARRAY_H_ */
