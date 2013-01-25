/*
 *  map.c
 *  Maper node as part of wordcount implementation based on mapreduce 32/128;
 *  Created on: 3.07.2012
 *      Author: YaroslavLitvinov
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "user_implem.h"
#include "mapreduce/map_reduce_lib.h"
#include "networking/eachtoother_comm.h"
#include "networking/channels_conf.h"
#include "networking/channels_conf_reader.h"
#include "defines.h"
#include "helpers/dyn_array.h"


int zmain(int argc, char **argv){
    /* argv[0] is node name
     * expecting in format : "name-%d",
     * format for single node without decimal id: "name" */
    int ownnodeid= -1;
    int extracted_name_len=0;
    int res =0;

    WRITE_FMT_LOG("Map node started argv[0]=%s.\n", argv[0] );

    /*get node type names via environnment*/
    char *map_node_type_text = getenv(ENV_MAP_NODE_NAME);
    char *red_node_type_text = getenv(ENV_REDUCE_NODE_NAME);
    assert(map_node_type_text);
    assert(red_node_type_text);

    ownnodeid = ExtractNodeNameId( argv[0], &extracted_name_len );
    /*nodename should be the same we got via environment and extracted from argv[0]*/
    assert( !strncmp(map_node_type_text, argv[0], extracted_name_len ) );
    if ( ownnodeid == -1 ){
	/*node id is not specified for single node and because assign nodeid=1*/
	ownnodeid=1; 
    }

    /*setup channels conf, now used static data but should be replaced by data from zrt*/
    struct ChannelsConfigInterface chan_if;
    SetupChannelsConfigInterface( &chan_if, ownnodeid, EMapNode );

    /***********************************************************************
     * setup network configuration of cluster: */

    /* add manifest channels to read from another map nodes */
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, 
						  IN_DIR, 
						  EChannelModeRead, 
						  EMapNode, 
						  map_node_type_text );
    assert( res == 0 );
    /* add manifest channels to read from another reduce nodes */
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, 
						  IN_DIR, 
						  EChannelModeRead, 
						  EReduceNode, 
						  red_node_type_text );
    assert( res == 0 );
    /* add manifest channels to write into another map nodes */
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, 
						  OUT_DIR, 
						  EChannelModeWrite, 
						  EMapNode, 
						  map_node_type_text );
    assert( res == 0 );
    /* add manifest channels to write into another reduce nodes */
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, 
						  OUT_DIR, 
						  EChannelModeWrite, 
						  EReduceNode, 
						  red_node_type_text );
    assert( res == 0 );
    /*add input channel into config*/
    res = chan_if.AddChannel( &chan_if, 
			      EInputOutputNode, 
			      1, /*nodeid*/
			      STDIN, /*associated input mode*/
			      EChannelModeRead ) != NULL? 0: -1;

    /*add fake channel into config, to access map nodes count later via
      channel interface at runtime; if we are not add this then mapreduce
      will fail because map nodes count will unavailable*/
    res = chan_if.AddChannel( &chan_if, 
			      EMapNode, 
			      ownnodeid, 
			      -1, 
			      EChannelModeRead ) != NULL? 0: -1;
    assert( res == 0 );
    /*--------------*/

    struct MapReduceUserIf mr_if;
    InitInterface( &mr_if );
    res = MapNodeMain(&mr_if, &chan_if); /*start map node*/

    /*mapreduce finished: map job complete*/
    CloseChannels(&chan_if);
    return res;
}
