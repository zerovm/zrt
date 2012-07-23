/*
 * common_channels_conf.h
 *
 *  Created on: 16.07.2012
 *      Author: yaroslav
 */

#ifndef COMMON_CHANNELS_CONF_H_
#define COMMON_CHANNELS_CONF_H_

/*this path is used to temporary store files that emulate communication channels,
 * for ZeroVM build it should use Zerovm channels implementation */
#define GCC_DEBUG_CHANNELS_PATH "debug/gcc/files/"
#define ZEROVM_DEBUG_CHANNELS_PATH ""

/*Stub initializes distributed app channels for all nodes*/
void FillUserChannelsList(struct ChannelsConfInterface *ch_if);
/*required only for debugging on fake network-files channels*/
void CloseChannels();

#endif /* COMMON_CHANNELS_CONF_H_ */
