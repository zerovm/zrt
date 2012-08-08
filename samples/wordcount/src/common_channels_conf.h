/*
 * common_channels_conf.h
 *
 *  Created on: 16.07.2012
 *      Author: yaroslav
 */

#ifndef COMMON_CHANNELS_CONF_H_
#define COMMON_CHANNELS_CONF_H_

#include <stddef.h>
#include <stdint.h>

struct ChannelsConfigInterface;

#define ENV_MAP_NODE_NAME "MAP_NAME"
#define ENV_REDUCE_NODE_NAME "REDUCE_NAME"
#define IN_DIR "/dev/in"
#define OUT_DIR "/dev/out"
#define MAX_PATH_LENGTH 255

int AddAllChannelsRelatedToNodeTypeFromDir( struct ChannelsConfigInterface *chan_if,
        const char *dirpath, int channel_mode, int nodetype, const char* nodename_text );

/* @param nameall string to extract values
 * @param nodenamelen length of extracted text up to trailing '-'
 * @return node id  */
int ExtractNodeNameId( const char *nameall, int *nodenamelen );

#endif /* COMMON_CHANNELS_CONF_H_ */
