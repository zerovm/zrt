/*
 * main_src.c
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "zrt.h"
#include "sort.h"
#include "defines.h"
#include "dsort.h"
#include "comm_dst.h"
#include "networking/channels_conf_reader.h"
#include "networking/channels_conf.h"

#define STDOUT 1

int start_node(struct ChannelsConfigInterface *chan_if, int nodeid){
    BigArrayPtr unsorted_array = NULL;
    BigArrayPtr sorted_array = NULL;

    /*receive sorted ranges from all source nodes*/
    unsorted_array = calloc( sizeof(BigArrayItem), ARRAY_ITEMS_COUNT );
    repreq_read_sorted_ranges( chan_if, unsorted_array, ARRAY_ITEMS_COUNT );

#ifdef MERGE_ON_FLY
    sorted_array = unsorted_array;
#else
    /*local sort of received pieces*/
    sorted_array = alloc_sort( unsorted_array, ARRAY_ITEMS_COUNT );
#endif

    /*save sorted array to output file*/
    const size_t data_size = sizeof(BigArrayItem)*ARRAY_ITEMS_COUNT;
    if ( sorted_array ){
        const ssize_t wrote = write( 1, sorted_array, data_size);
        WRITE_FMT_LOG( LOG_DEBUG, "wrote=%d, data_size=%d\n", wrote, data_size );
        assert(wrote == data_size );
    }

    /*send crc to manager node*/
    struct UserChannel *channel = chan_if->Channel(chan_if, EManagerNode, 1, EChannelModeWrite);
    write_sort_result( channel->fd, nodeid, sorted_array, ARRAY_ITEMS_COUNT );

    free(unsorted_array);
    WRITE_LOG( LOG_UI,  "Destination node complete\n");
    return 0;
}

int zmain(int argc, char **argv){
    /* argv[0] is node name
     * expecting in format : "name-%d",
     * format for single node without decimal id: "name" */
    WRITE_FMT_LOG(LOG_DEBUG, "Destination node started argv[0]=%s.\n", argv[0] );

    if ( argc < 2 ){
	WRITE_LOG(LOG_ERR, "argv[1] is expected, items count need to be passed.\n" );
	return -1;
    }
    set_items_count_to_sortjob( atoi(argv[1]) );

    int ownnodeid= -1;
    int extracted_name_len=0;
    int res =0;

    /*get node type names via environnment*/
    char *dest_node_type_text = getenv(ENV_DEST_NODE_NAME);
    char *source_node_type_text = getenv(ENV_SOURCE_NODE_NAME);
    char *man_node_type_text = getenv(ENV_MAN_NODE_NAME);
    assert(dest_node_type_text);
    assert(source_node_type_text);
    assert(man_node_type_text);

    ownnodeid = ExtractNodeNameId( argv[0], &extracted_name_len );
    /*nodename should be the same we got via environment and extracted from argv[0]*/
    assert( strncmp(dest_node_type_text, argv[0], extracted_name_len ) == 0 );
    if ( ownnodeid == -1 ) ownnodeid=1; /*node id not specified for single node by default assign nodeid=1*/

    /*setup channels conf, now used static data but should be replaced by data from zrt*/
    struct ChannelsConfigInterface chan_if;
    SetupChannelsConfigInterface( &chan_if, ownnodeid, EDestinationNode );

    /***********************************************************************
      Add channels configuration into config object */
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, IN_DIR, EChannelModeRead, ESourceNode, source_node_type_text );
    assert( res == 0 );
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, OUT_DIR, EChannelModeWrite, EManagerNode, man_node_type_text );
    assert( res == 0 );
    /*add input channel into config*/
    res = chan_if.AddChannel( &chan_if, EInputOutputNode, 1, STDOUT, EChannelModeWrite ) != NULL? 0: -1;
    assert( res == 0 );
    /*--------------*/

    start_node(&chan_if, ownnodeid);
    CloseChannels(&chan_if);
    return 0;
}

