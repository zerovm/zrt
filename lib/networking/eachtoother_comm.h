/*
 * eachtoother_comm.h
 *
 *  Created on: 14.07.2012
 *      Author: yaroslav
 */

#ifndef MAP_COMMUNICATION_PATTERN_H_
#define MAP_COMMUNICATION_PATTERN_H_

#include <stddef.h> //size_t
#include "channels_conf.h"

/*Pattern for dermenistic claster nodes network communications without manager node*/
struct EachToOtherPattern{
	struct ChannelsConfigInterface *conf;
	void *data;
	/*index - index in nodes_list currently reading*/
	void (*Read)( struct EachToOtherPattern *p_this, int nodetype, int index, int fdr );
	/*index - index in nodes_list currently writing*/
	void (*Write)( struct EachToOtherPattern *p_this, int nodetype, int index, int fdw );
};

/*EachToOtherPattern user function should be configured*/
void StartEachToOtherCommunication( struct EachToOtherPattern* p_this, int nodetype );

#endif /* MAP_COMMUNICATION_PATTERN_H_ */
