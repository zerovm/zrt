/*
 * map_communication_pattern.c
 *
 *  Created on: 14.07.2012
 *      Author: yaroslav
 */

#include <assert.h>
#include "eachtoother_comm.h"
#include "defines.h"

void StartEachToOtherCommunication( struct EachToOtherPattern* p_this, int nodetype ){
	assert( p_this );
	assert( p_this->conf );
	int nodes_count;
	int *nodes_list = p_this->conf->NodesList( p_this->conf, nodetype, &nodes_count );
	assert(nodes_count);

	/*For every map node in list*/
	for ( int i=0; i < nodes_count; i++ ){
		if ( nodes_list[i] == p_this->conf->ownnodeid ){
			/*send data to all another nodes in list*/
			for ( int j=0; j < nodes_count; j++ ){
				/*do not send data to himself*/
				if ( nodes_list[j] != p_this->conf->ownnodeid ){
					p_this->Write(
							p_this,
							nodetype,
							j,
							p_this->conf->ChannelConfFd( p_this->conf, nodes_list[j], CHANNEL_MODE_WRITE) );
				}
			}
		}
		if ( nodes_list[i] != p_this->conf->ownnodeid ){
			/*read from all another nodes*/
			p_this->Read(
					p_this,
					nodetype,
					i,
					p_this->conf->ChannelConfFd( p_this->conf, nodes_list[i], CHANNEL_MODE_READ ) );
		}
	}
}

