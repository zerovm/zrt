/*
 * eachtoother_comm.c
 *
 *  Created on: 14.07.2012
 *      Author: yaroslav
 */

#include <stdlib.h>
#include <assert.h>
#include "eachtoother_comm.h"
#include "defines.h"

void StartEachToOtherCommunication( struct EachToOtherPattern* p_this, int nodetype ){
	assert( p_this );
	assert( p_this->conf );
	int *nodes_list = NULL;
	int nodes_count = p_this->conf->GetNodesListByType( p_this->conf, nodetype, &nodes_list );
	struct UserChannel *channel = NULL;
	assert(nodes_count);

	/*For every map node in list*/
	for ( int i=0; i < nodes_count; i++ ){
		if ( nodes_list[i] == p_this->conf->ownnodeid ){
			/*send data to all another nodes in list*/
			for ( int j=0; j < nodes_count; j++ ){
			    channel = p_this->conf->Channel(p_this->conf, nodetype, nodes_list[j], EChannelModeWrite );
				/*do not send data to himself*/
				if ( nodes_list[j] != p_this->conf->ownnodeid ){
				    assert(channel);
					p_this->Write( p_this, nodetype, j, channel->fd );
				}
			}
		}
		if ( nodes_list[i] != p_this->conf->ownnodeid ){
            channel = p_this->conf->Channel(p_this->conf, nodetype, nodes_list[i], EChannelModeRead );
            assert(channel);
			/*read from all another nodes*/
			p_this->Read( p_this, nodetype, i, channel->fd );
		}
	}
	free(nodes_list);
}

