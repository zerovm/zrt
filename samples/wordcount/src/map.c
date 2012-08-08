/*
 * main_test1.c
 *
 *  Created on: 3.07.2012
 *      Author: YaroslavLitvinov
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#ifdef USER_SIDE
	#include "zrt.h"
#endif
#include "user_implem.h"
#include "map_reduce_lib.h"
#include "eachtoother_comm.h"
#include "channels_conf.h"
#include "common_channels_conf.h" //temp
#include "defines.h"
#include "helpers/dyn_array.h"


int main(int argc, char **argv){
    /* argv[0] is node name
     * expecting in format : "name-%d",
     * format for single node without decimal id: "name" */
    int ownnodeid= -1;
    int extracted_name_len=0;
    int res =0;

	WRITE_FMT_LOG("Map node started argv[0]=%s.\n", argv[1] );

	/*get node type names via environnment*/
    char *map_node_type_text = getenv(ENV_MAP_NODE_NAME);
    char *red_node_type_text = getenv(ENV_REDUCE_NODE_NAME);
    assert(map_node_type_text);
    assert(red_node_type_text);

	ownnodeid = ExtractNodeNameId( argv[1], &extracted_name_len );
	/*nodename should be the same we got via environment and extracted from argv[0]*/
	assert( !strncmp(map_node_type_text, argv[1], extracted_name_len ) );
	if ( ownnodeid == -1 ) ownnodeid=1; /*node id not specified for single node by default assign nodeid=1*/

    /*setup channels conf, now used static data but should be replaced by data from zrt*/
    struct ChannelsConfigInterface chan_if;
    SetupChannelsConfigInterface( &chan_if, ownnodeid, EMapNode );

    /***********************************************************************
     Add channels configuration into config object */
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, IN_DIR, EChannelModeRead, EMapNode, map_node_type_text );
    assert( res == 0 );
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, IN_DIR, EChannelModeRead, EReduceNode, red_node_type_text );
    assert( res == 0 );
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, OUT_DIR, EChannelModeWrite, EMapNode, map_node_type_text );
    assert( res == 0 );
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, OUT_DIR, EChannelModeWrite, EReduceNode, red_node_type_text );
    assert( res == 0 );
    /*add input channel into config*/
    res = chan_if.AddChannel( &chan_if, EInputOutputNode, 1, 0, EChannelModeRead ) != NULL? 0: -1;
    /*add fake channel into config related to mapnode types, to get right nodes list*/
    res = chan_if.AddChannel( &chan_if, EMapNode, ownnodeid, -1, EChannelModeRead ) != NULL? 0: -1;
    assert( res == 0 );
	/*--------------*/

	struct MapReduceUserIf mr_if;
	memset( &mr_if, '\0', sizeof(mr_if) );
	InitInterface( &mr_if );
	mr_if.data.keytype = EUint32;
	mr_if.data.valuetype = EUint32;
	res = MapNodeMain(&mr_if, &chan_if);

	/*map job complete*/
    for ( int i=0; i < chan_if.channels->num_entries;i++ ){
        struct UserChannel *channel = (struct UserChannel *)chan_if.channels->ptr_array[i];
        if ( channel ){
            close(channel->fd);  /*close all opened file descriptors*/
        }
    }
    chan_if.Free(&chan_if); /*freee memories used by channels configuration*/

	return res;
}
