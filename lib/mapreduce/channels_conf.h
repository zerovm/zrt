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


#ifndef CHANNELS_CONF_H_
#define CHANNELS_CONF_H_

#include <stdint.h>

/*User at first should configure own claster network, and in further can get configuration
 * via interface accessors : ChannelConfFd, ChannelConfPath */

/*Theses list intended For UserChannel::dstnodeid to map stdin into generic file decriptor*/
#define STDIN 0

typedef enum  ChannelMode{ EChannelModeRead=0, EChannelModeWrite=1 }  ChannelMode;
struct UserChannel{
    uint8_t        nodetype;
	uint16_t       nodeid;  /*unique for same nodetype records*/
    uint16_t        fd;     /*unique*/
	ChannelMode    mode; /*EChannelModeRead, EChannelModeWrite*/
};

struct ChannelsConfigInterface{
	/*Add nodes list related to same type,
	 * nodetype should be value in range from 0 up to node types count
	 * nodeid  should be unique for the same node type*/
    struct UserChannel *(*AddChannel)( struct ChannelsConfigInterface *ch_if,
            int nodetype, int nodeid, int channelfd, ChannelMode mode );
	/*Get nodes list by nodetype
	 * @param  pointer to list
	 * @return nodes_count get nodes count*/
    int (*GetNodesListByType)( const struct ChannelsConfigInterface *ch_if, int nodetype, int **nodes_array );
	/*get channel config*/
    struct UserChannel *(*Channel)(struct ChannelsConfigInterface *ch_if,
            int nodetype, int nodeid, int8_t channelmode);
	/*free resources*/
	void (*Free)(struct ChannelsConfigInterface *ch_if);

	/*******************************************************
	 * data */
	int ownnodeid;
	int ownnodetype;
	/*private data*/
	struct DynArray* channels;
};

void SetupChannelsConfigInterface( struct ChannelsConfigInterface *ch_if, int ownnodeid, int ownnodetype );





#endif /* CHANNELS_CONF_H_ */
