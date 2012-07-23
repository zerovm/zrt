/*
 * channels_conf.h
 *
 *  Created on: 15.07.2012
 *      Author: yaroslav
 */

#ifndef CHANNELS_CONF_H_
#define CHANNELS_CONF_H_

#include <stdint.h>

/*User at first should configure own claster network, and next use conf via accessors
 * ChannelConfFd, ChannelConfPath */

#define CHANNEL_MODE_READ ('r')
#define CHANNEL_MODE_WRITE ('w')

/*Theses list intended For UserChannel::dstnodeid to map stdin into generic file decriptor*/
#define STDIN 0

struct UserChannel{
	uint16_t srcnodeid;
	uint16_t dstnodeid;
	int16_t fd;
	int8_t mode; /*CHANNEL_MODE_READ | CHANNEL_MODE_WRITE*/
	char *path;
};


struct UserChannelInternal{
	struct UserChannel user_channel;
	struct UserChannelInternal *next;
};

struct NodesListInternal{
	int nodetype;
	int nodes_count;
	int *nodes_list;
};

struct ChannelsConfInterface{
	int (*TestConf)( struct ChannelsConfInterface *ch_if );
	/*Add nodes list related to same type*/
	void (*NodesListAdd)( struct ChannelsConfInterface *ch_if, int nodetype, int *nodes_list, int nodes_count );
	/*Get nodes list by nodetype
	 * @param nodes_count get nodes count
	 * @return pointer to list*/
	int *(*NodesList)( const struct ChannelsConfInterface *ch_if, int nodetype, int *nodes_count );
	/*Add channels to configuration one by one. UserChannel::path should be acessible during lifecycle*/
	void (*ChannelConfAdd)(struct ChannelsConfInterface *ch_if, struct UserChannel *channel );
	/*get conf data*/
	int (*ChannelConfFd)(struct ChannelsConfInterface *ch_if, int nodeid, int8_t mode);
	/*get conf data*/
	char* (*ChannelConfPath)(struct ChannelsConfInterface *ch_if, int nodeid, int8_t mode);
	/*free resources*/
	void (*Free)(struct ChannelsConfInterface *ch_if);
	/*******************************************************
	 * data */
	int ownnodeid; /*For every node ownnodeid should be different*/

	/*private data*/
	struct UserChannelInternal *user_channels_internal; /*should not be used directly*/
	struct NodesListInternal *nodes_list_internal; /*should not be used directly*/
	int nodes_list_count_internal; /*should not be used directly*/
};

void SetupChannelsConfInterface( struct ChannelsConfInterface *ch_if, int ownnodeid  );





#endif /* CHANNELS_CONF_H_ */
