/*
 * channels_conf.c
 *
 *  Created on: 16.07.2012
 *      Author: yaroslav
 */

#include <string.h> //memset
#include <stdlib.h> //calloc
#include <stddef.h>
#include <assert.h>

#include "channels_conf.h"

/*helper to channels navigate*/
static struct UserChannelInternal **__next_chann = NULL;

int TestConf( struct ChannelsConfInterface *ch_if ){
	return 1;
}

void Free(struct ChannelsConfInterface *ch_if){
	/*free nodes lists memories*/
	free(ch_if->nodes_list_internal);
	struct UserChannelInternal *next = ch_if->user_channels_internal;
	do {
		if ( next ){
			struct UserChannelInternal *current = next;
			next = next->next;
			free( current );
		}
	}while( next != NULL );
}

void NodesListAdd(struct ChannelsConfInterface *ch_if, int nodetype, int *nodes_list, int nodes_count ){
	if ( ! ch_if->nodes_list_internal ){
		ch_if->nodes_list_internal = malloc( sizeof(struct NodesListInternal) );
		ch_if->nodes_list_count_internal = 0;
	}
	else{
		ch_if->nodes_list_internal = realloc(
				ch_if->nodes_list_internal,
				sizeof(struct NodesListInternal)*(ch_if->nodes_list_count_internal+1) );
	}
	ch_if->nodes_list_internal[ch_if->nodes_list_count_internal].nodetype = nodetype;
	ch_if->nodes_list_internal[ch_if->nodes_list_count_internal].nodes_count = nodes_count;
	ch_if->nodes_list_internal[ch_if->nodes_list_count_internal].nodes_list = nodes_list;
	++ch_if->nodes_list_count_internal;
}

int *NodesList( const struct ChannelsConfInterface *ch_if, int nodetype, int *nodes_count ){
	if ( ch_if->nodes_list_internal ){
		for( int i=0; i < ch_if->nodes_list_count_internal; i++ ){
			if ( ch_if->nodes_list_internal[i].nodetype == nodetype ){
				*nodes_count = ch_if->nodes_list_internal[i].nodes_count;
				return ch_if->nodes_list_internal[i].nodes_list;
			}
		}
	}
	*nodes_count = 0;
	return NULL;
}

void ChannelConfAdd( struct ChannelsConfInterface *ch_if, struct UserChannel *user_channel ){
	/*add first time*/
	if ( !ch_if->user_channels_internal ){
		ch_if->user_channels_internal = calloc( 1, sizeof(struct UserChannelInternal) );
		__next_chann = &ch_if->user_channels_internal;
	}
	else{
		/*add aggain*/
		*__next_chann = calloc( 1, sizeof(struct UserChannelInternal) );
	}
	(*__next_chann)->user_channel = *user_channel;
	(*__next_chann)->next = NULL;
	__next_chann = &(*__next_chann)->next;
}


struct UserChannel* SearchByNodeid(struct ChannelsConfInterface *ch_if, int nodeid, int8_t mode ){
	struct UserChannelInternal *next = ch_if->user_channels_internal;
	do {
		if ( next ){
			if ( next->user_channel.srcnodeid == ch_if->ownnodeid &&
				 next->user_channel.dstnodeid == nodeid &&
				 next->user_channel.mode == mode )
			{
				return &next->user_channel;
			}
			next = next->next;
		}
	}while( next != NULL );
	return NULL;
}



int ChannelConfFd(struct ChannelsConfInterface *ch_if, int nodeid, int8_t mode){
	struct UserChannel *ch = SearchByNodeid( ch_if, nodeid, mode );
	assert(ch);
	return ch->fd;
}

char* ChannelConfPath(struct ChannelsConfInterface *ch_if, int nodeid, int8_t mode){
	struct UserChannel *ch = SearchByNodeid( ch_if, nodeid, mode );
	assert(ch);
	return ch->path;
}


void SetupChannelsConfInterface( struct ChannelsConfInterface *ch_if, int ownnodeid  ){
	memset( ch_if, '\0', sizeof(*ch_if) );
	ch_if->TestConf = TestConf;
	ch_if->NodesListAdd = NodesListAdd;
	ch_if->NodesList = NodesList;
	ch_if->ChannelConfAdd = ChannelConfAdd;
	ch_if->ChannelConfFd = ChannelConfFd;
	ch_if->ChannelConfPath = ChannelConfPath;
	ch_if->ownnodeid = ownnodeid;
}

