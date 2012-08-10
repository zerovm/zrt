/*
 * channels_conf.h
 *
 *  Created on: 15.07.2012
 *      Author: yaroslav
 */

#ifndef CHANNELS_CONF_H_
#define CHANNELS_CONF_H_

#include <stdint.h>
#include <stdio.h>


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
	void (*DebugPrint)( struct UserChannel *channel, FILE* out  );
};
void DebugPrint( struct UserChannel *channel, FILE* out );

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
