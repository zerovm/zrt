/*
 * MountsObserver.h
 *
 *  Created on: 19.09.2012
 *      Author: yaroslav
 */

#ifndef MOUNTSOBSERVER_H_
#define MOUNTSOBSERVER_H_

struct ZVMChannel;

struct MountsObserver{
    int (*is_channel_handle)(int handle);
    const struct ZVMChannel *(*channels_list)(int* channels_count);
    int (*channel_handle)(const char* path);
    const struct ZVMChannel* (*channel)(int handle);
};

struct MountsObserver* alloc_mounts_observer( const struct ZVMChannel *channels, int channels_count );

#endif /* MOUNTSOBSERVER_H_ */
