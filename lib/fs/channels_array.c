/*
 *
 * Copyright (c) 2013, LiteStack, Inc.
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "zvm.h" //struct ZVMChannel
#include "zrtlog.h"
#include "channels_array.h"
#include "channels_mount.h" //INODE_FROM_ZVM_INODE
#include "channel_array_item.h" //struct ChannelArrayItem
#include "dyn_array.h" //DynArray

#define EMU_CHANNELS    1
#define ZVM_CHANNELS 0
#define ADD_CHANNELS(channels_if_p, channels_array_p, count, check){	\
	/*add channels from array*/					\
	struct ChannelArrayItem* item;					\
	int i;							\
	for (i=0; i < count; i++){					\
	    if ( (check) == EMU_CHANNELS &&				\
		 NULL != (channels_if_p)->public.match_by_name(&(channels_if_p)->public, (channels_array_p)[i].name ) ){ \
		continue; /*do not add matched channel*/		\
	    }								\
	    /*alloc item*/						\
	    item = malloc(sizeof(struct ChannelArrayItem));		\
	    item->channel = &(channels_array_p)[i];			\
	    item->channel_runtime.inode =				\
		INODE_FROM_ZVM_INODE((channels_if_p)->array.num_entries); \
	    if ( (check) == EMU_CHANNELS ){				\
		item->channel_runtime.emu = 1;				\
	    }								\
	    DynArraySet( &(channels_if_p)->array,			\
			 (channels_if_p)->array.num_entries, item );	\
	    assert( res != 0 );						\
	}								\
    }


struct ChannelsArray{
    //base, it is must be a first member
    struct ChannelsArrayPublicInterface public;
    /*private data*/
    struct DynArray array;
};



int channels_array_count(struct ChannelsArray* this){
    return this->array.num_entries;
}

struct ChannelArrayItem* channels_array_get(struct ChannelsArray* this, int index){
    return DynArrayGet(&this->array, index);
}

struct ChannelArrayItem* channels_array_match_by_name(struct ChannelsArray* this, 
						      const char* channel_name ){
    /* search for name through the channels list*/
    int handle;
    struct ChannelArrayItem* item;
    for(handle = 0; handle < this->array.num_entries; ++handle){
	item = DynArrayGet(&this->array, handle);
	if( strcmp( item->channel->name, channel_name) == 0){
	    return item; /*matched item*/
	}
    }
    
    return NULL; /* if channel name not matched return error*/
}

struct ChannelArrayItem* channels_array_match_by_inode(struct ChannelsArray* this, 
						      int inode ){
    /* search for inode through the channels list*/
    int idx;
    struct ChannelArrayItem* item;
    for(idx = 0; idx < this->array.num_entries; ++idx){
	item = DynArrayGet(&this->array, idx);
	ZRT_LOG( L_EXTRA, "inode=%d; name=%10s", 
		 item->channel_runtime.inode, item->channel->name );
	if( item->channel_runtime.inode == inode){
	    return item; /*matched item*/
	}
    }
    
    return NULL; /* if not matched return error*/
}


/* void channels_array_refresh_channels(struct ChannelsArray* this){ */
/*     for( int i=0; i < this->array->num_entries; i++ ){ */
/*         free( DynArrayGet(this->array, i) ); */
/*     } */
/*     DynArrayDtor( this->array ); */
/* } */

struct ChannelsArrayPublicInterface* 
channels_array_construct( const struct ZVMChannel* zvm_channels, 
			  int zvm_channels_count,
			  const struct ZVMChannel* emu_channels,
			  int emu_channels_count){
    /*manifest param contain channels list, channels may refreshed at
      any time by zerovm, so we will refer to manifest object at
      future calls, but no need to save manifest pointer because we
      have fixed memory address of manifest object, that provided by
      zerovm macros MANIFEST*/
    /*create returning main objest in heap*/
    struct ChannelsArray* this = malloc(sizeof(struct ChannelsArray));

    /*set functions*/
    this->public.count = (void*)channels_array_count;
    this->public.get   = (void*)channels_array_get;
    this->public.match_by_name = (void*)channels_array_match_by_name;
    this->public.match_by_inode = (void*)channels_array_match_by_inode;
    /*alloc array & set data members*/
    int res = DynArrayCtor( &this->array, 
			   zvm_channels_count+emu_channels_count);
    assert( res != 0 );

    /*add zvm_channels*/
    ADD_CHANNELS (this, zvm_channels, zvm_channels_count, ZVM_CHANNELS);
    /*add emu_channels*/
    ADD_CHANNELS (this, emu_channels, emu_channels_count, EMU_CHANNELS);
    /*test item added*/
    assert( this->array.num_entries == zvm_channels_count+emu_channels_count );
    
    return (struct ChannelsArrayPublicInterface*)this;
}


