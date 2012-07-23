/*
 * main_test1.c
 *
 *  Created on: 3.07.2012
 *      Author: YaroslavLitvinov
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef USER_SIDE
	#include "zrt.h"
#endif
#include "user_implem.h"
#include "map_reduce_lib.h"
#include "eachtoother_comm.h"
#include "channels_conf.h"
#include "common_channels_conf.h" //temp
#include "defines.h"


/*should be accessible by all node type by single reference*/
const int __map_nodes_list[] = { 1, 2, 3, 4 }; /*ascending order*/
const int __reduce_nodes_list[] = { 5, 6, 7, 8, 9 }; /*ascending order*/


int main(int argc, char **argv){
	WRITE_LOG("Map node started.\n" );
	if ( argc < 2 ){
		WRITE_LOG("Required 1 arg unique node integer id\n");
		exit(-1);
	}
	const int nodeid = atoi(argv[1]);
	WRITE_FMT_LOG("nodeid=%d \n", nodeid);

	/*setup channels conf, now used static data but should be replaced by data from zrt*/
	struct ChannelsConfInterface chan_if;
	SetupChannelsConfInterface( &chan_if, nodeid );
	chan_if.NodesListAdd( &chan_if, EMapNode,
			(int*)__map_nodes_list, sizeof(__map_nodes_list)/sizeof(*__map_nodes_list) );
	chan_if.NodesListAdd( &chan_if, EReduceNode,
			(int*)__reduce_nodes_list, sizeof(__reduce_nodes_list)/sizeof(*__reduce_nodes_list) );
	FillUserChannelsList( &chan_if ); /*temp. setup distributed conf, by static data */
	/*--------------*/

	struct MapReduceUserIf mr_if;
	memset( &mr_if, '\0', sizeof(mr_if) );
	InitInterface( &mr_if );
	mr_if.data.keytype = EUint32;
	mr_if.data.valuetype = EUint32;
	int res = MapNodeMain(&mr_if, &chan_if);

	CloseChannels();

	return res;
}
