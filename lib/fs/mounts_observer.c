/*
 * MountsObserver.c
 *
 *  Created on: 19.09.2012
 *      Author: yaroslav
 */

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

#include "zvm.h"
#include "mounts_observer.h"

const struct ZVMChannel *s_channels = NULL;
int s_channels_count;


const struct ZVMChannel* channels_list(int* channels_count){
    *channels_count = s_channels_count;
    return s_channels;
}

int is_channel_handle(int handle){
    return (handle>=0 && handle < s_channels_count) ? 1 : 0;
}

const struct ZVMChannel* channel(int handle){
    assert( handle >=0 && handle < s_channels_count );
    return &s_channels[handle];
}

int channel_handle(const char* path){
    const struct ZVMChannel* chan = NULL;

    /* search for name through the channels list*/
    int handle;
    for(handle = 0; handle < s_channels_count; ++handle)
        if(strcmp(s_channels[handle].name, path) == 0){
            chan = &s_channels[handle];
            break; /*handle according to matched filename*/
        }

    if ( !chan )
        return -1; /* if channel name not matched return error*/
    else
        return handle;
}

struct MountsObserver* alloc_mounts_observer( const struct ZVMChannel *channels, int channels_count ){
    s_channels = channels;
    s_channels_count = channels_count;

    struct MountsObserver* observer = malloc(sizeof(struct MountsObserver));
    /*setup all observer functions, that facade MountsObserver contains*/
    observer->channels_list     = channels_list;
    observer->channel_handle    = channel_handle;
    observer->is_channel_handle = is_channel_handle;
    observer->channel           = channel;
    return observer;
}
