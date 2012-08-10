/*
 * common_channels_conf.h
 *
 *  Created on: 16.07.2012
 *      Author: yaroslav
 */

#ifndef CHANNELS_CONF_READER_H_
#define CHANNELS_CONF_READER_H_

#include <stddef.h>
#include <stdint.h>

struct ChannelsConfigInterface;

#define IN_DIR "/dev/in"
#define OUT_DIR "/dev/out"

int AddAllChannelsRelatedToNodeTypeFromDir( struct ChannelsConfigInterface *chan_if,
        const char *dirpath, int channel_mode, int nodetype, const char* nodename_text );

/*Close added and opened channels*/
void CloseChannels( struct ChannelsConfigInterface *chan_if );

/* @param nameall string to extract values
 * @param nodenamelen length of extracted text up to trailing '-'
 * @return node id  */
int ExtractNodeNameId( const char *nameall, int *nodenamelen );

#endif /* CHANNELS_CONF_READER_H_ */
