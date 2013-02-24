/*
 * manager.c
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "zrt.h"
#include "histanlz.h"
#include "defines.h"
#include "dsort.h"
#include "distr_common.h"
#include "comm_man.h"
#include "networking/channels_conf_reader.h"
#include "networking/channels_conf.h"

#define STDOUT 1

int start_node(struct ChannelsConfigInterface *chan_if){
    int *src_nodes_list = NULL;
    int src_nodes_count = chan_if->GetNodesListByType(chan_if, ESourceNode, &src_nodes_list);

    /*crc reading from all source nodes*/
    uint32_t *crc_array = malloc(src_nodes_count*sizeof(uint32_t));
    memset(crc_array, '\0', src_nodes_count*sizeof(uint32_t));
    read_crcs(chan_if, src_nodes_list, crc_array, src_nodes_count);
    /*crc read ok*/

    /******** Histograms reading*/
    struct Histogram histograms[src_nodes_count];
    WRITE_LOG(LOG_DEBUG, "Recv histograms");
    recv_histograms( chan_if, src_nodes_list, (struct Histogram*) &histograms, src_nodes_count );
    WRITE_LOG(LOG_DEBUG, "Recv histograms OK");
    /******** Histograms reading OK*/

    /*Sort histograms by src_nodeid, because recv order is unexpected,
     * but histogram analizer algorithm is required deterministic order*/
    qsort( histograms, src_nodes_count, sizeof(struct Histogram), histogram_srcid_comparator );

    WRITE_LOG(LOG_DEBUG, "Analize histograms and request detailed histograms\n");
    struct request_data_t** range = alloc_range_request_analize_histograms( chan_if,
            ARRAY_ITEMS_COUNT, histograms, src_nodes_count );
    WRITE_LOG(LOG_DEBUG, "Analize histograms and request detailed histograms OK\n");

    for (int i=0; i < src_nodes_count; i++ )
    {
        WRITE_FMT_LOG(LOG_DEBUG, "SOURCE PART N %d:\n", i );
        print_request_data_array( range[i], src_nodes_count );
    }

    WRITE_LOG(LOG_DETAILED_UI, "Send range requests to source nodes\n" );

    /// sending range requests to every source node
    for (int i=0; i < src_nodes_count; i++ ){
        struct UserChannel *channel = chan_if->Channel(chan_if, ESourceNode, range[0][i].src_nodeid, EChannelModeWrite );
        assert(channel);
#if LOG_LEVEL == LOG_DEBUG
        channel->DebugPrint(channel, stderr);
#endif
        WRITE_FMT_LOG(LOG_DEBUG, "write_sorted_ranges fdw=%d\n", channel->fd );
        write_range_request( channel->fd, range, src_nodes_count, i );
    }

    for ( int i=0; i < src_nodes_count; i++ ){
        free(histograms[i].array);
        free( range[i] );
    }
    free(range);

    WRITE_LOG(LOG_DEBUG, "Read confirmation sort results, to test it\n");

    struct sort_result *results = read_sort_result( chan_if );
    /*sort results by nodeid, because receive order not deterministic*/
    qsort( results, src_nodes_count, sizeof(struct sort_result), sortresult_comparator );
    int sort_ok = 1;
    uint32_t crc_test_unsorted = 0;
    uint32_t crc_test_sorted = 0;
    for ( int i=0; i < src_nodes_count; i++ ){
        crc_test_unsorted = (crc_test_unsorted+crc_array[i]% CRC_ATOM) % CRC_ATOM;
        crc_test_sorted = (crc_test_sorted+results[i].crc% CRC_ATOM) % CRC_ATOM;
        if ( i>0 ){
            if ( !(results[i].max > results[i].min && results[i-1].max < results[i].min) )
                sort_ok = 0;
        }
        WRITE_FMT_LOG(LOG_DEBUG, "results[%d], nodeid=%d, min=%u, max=%u\n", i, results[i].nodeid, results[i].min, results[i].max);
        fflush(0);
    }

    WRITE_FMT_LOG(LOG_UI, "Distributed sort complete, Test %d, %d, %d\n", sort_ok, crc_test_unsorted, crc_test_sorted );
    if ( crc_test_unsorted == crc_test_sorted ){
        WRITE_LOG(LOG_UI, "crc OK\n");
    }
    else{
        WRITE_LOG(LOG_UI,"crc FAILED\n");
    }
    return 0;
}

int main(int argc, char **argv){
    WRITE_FMT_LOG(LOG_DEBUG, "Manager node started argv[0]=%s.\n", argv[0] );
    if ( argc < 2 ){
	WRITE_LOG(LOG_ERR, "argv[1] is expected, items count need to be passed.\n" );
	return -1;
    }
    set_items_count_to_sortjob( atoi(argv[1]) );
    int res =0;

    /*get node type names via environnment*/
    char *source_node_type_text = getenv(ENV_SOURCE_NODE_NAME);
    char *dest_node_type_text = getenv(ENV_DEST_NODE_NAME);
    assert(source_node_type_text);
    assert(dest_node_type_text);

    /*setup channels conf, now used static data but should be replaced by data from zrt*/
    struct ChannelsConfigInterface chan_if;
    SetupChannelsConfigInterface( &chan_if, 1/*nodeid*/, EManagerNode );

    /***********************************************************************
     Add channels configuration into config object */
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, IN_DIR, EChannelModeRead, ESourceNode, source_node_type_text );
    assert( res == 0 );
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, IN_DIR, EChannelModeRead, EDestinationNode, dest_node_type_text );
    assert( res == 0 );
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, OUT_DIR, EChannelModeWrite, ESourceNode, source_node_type_text );
    assert( res == 0 );
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, OUT_DIR, EChannelModeWrite, EDestinationNode, dest_node_type_text );
    assert( res == 0 );
    /*add input channel into config*/
    res = chan_if.AddChannel( &chan_if, EInputOutputNode, 1, STDOUT, EChannelModeWrite ) != NULL? 0: -1;
    assert( res == 0 );
    /*--------------*/

    start_node(&chan_if);
    CloseChannels(&chan_if);
    return 0;
}
