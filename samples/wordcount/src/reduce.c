/*
 *  reduce.c
 *  Reducer node as part of wordcount implementation based on mapreduce 32/128;
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
#include "channels_conf_reader.h"
#include "defines.h"
#include "helpers/dyn_array.h"


int zmain(int argc, char **argv){
    /* argv[0] is node name
     * expecting in format : "name-%d",
     * format for single node without decimal id: "name" */
    int ownnodeid= -1;
    int extracted_name_len=0;
    int res =0;

    WRITE_FMT_LOG("Reduce node started argv[0]=%s.\n", argv[1] );

    /*get node type names via environnment*/
    char *map_node_type_text = getenv(ENV_MAP_NODE_NAME);
    char *red_node_type_text = getenv(ENV_REDUCE_NODE_NAME);
    assert(map_node_type_text);
    assert(red_node_type_text);

    ownnodeid = ExtractNodeNameId( argv[1], &extracted_name_len );
    /*nodename should be the same we got via environment and extracted from argv[0]*/
    assert( !strncmp(red_node_type_text, argv[1], extracted_name_len ) );
    if ( ownnodeid == -1 ) ownnodeid=1; /*node id not specified for single node by default assign nodeid=1*/

    /*setup channels conf, now used static data but should be replaced by data from zrt*/
    struct ChannelsConfigInterface chan_if;
    SetupChannelsConfigInterface( &chan_if, ownnodeid, EReduceNode );

    /***********************************************************************
     * setup network configuration of cluster: */

    /* add manifest channels to read from map nodes */
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, 
						  IN_DIR, 
						  EChannelModeRead, 
						  EMapNode,
						  map_node_type_text );
    assert( res == 0 );
    /*associate stdout with results output of reduce node*/
    res = chan_if.AddChannel( &chan_if, 
			      EInputOutputNode, 
			      EReduceNode, 
			      STDOUT, 
			      EChannelModeWrite ) != NULL? 0: -1;

    assert( res == 0 );
    /*--------------*/

    struct MapReduceUserIf mr_if;
    InitInterface( &mr_if );
    res = ReduceNodeMain(&mr_if, &chan_if); /*start reduce node*/

    WRITE_LOG("complete---------------------");

    /*mapreduce finished: reduce job complete*/    
    CloseChannels(&chan_if);
    return res;
}
