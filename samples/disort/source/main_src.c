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
#include "dsort.h"
#include "defines.h"
#include "cpuid.h"
#include "comm_src.h"
#include "channels_conf_reader.h"
#include "channels_conf.h"
#include "bitonic_sort.h"


#define STDIN 0

static int
quicksort_reqdata_by_destnodeid_comparator( const void *m1, const void *m2 ){

    const struct request_data_t *t1= (const struct request_data_t*)(m1);
    const struct request_data_t *t2= (const struct request_data_t*)(m2);

    if ( t1->dst_nodeid < t2->dst_nodeid )
        return -1;
    else if ( t1->dst_nodeid > t2->dst_nodeid )
        return 1;
    else return 0;
    return 0;
}

/* need for SSE aligned memory access */
void *aligned_malloc(size_t size, size_t align)
{
    void *mem = malloc(size + align + sizeof(void*));
    void **ptr = (void**)(((size_t)mem + align + sizeof(void*)) & ~(align - 1));
    ptr[-1] = mem;
    return ptr;
}

/* replaces free() */
void aligned_free(void *ptr)
{
    free(((void**)ptr)[-1]);
}


int start_node(  struct ChannelsConfigInterface *chan_if, int nodeid ){
    BigArrayPtr sorted_array = NULL;
    const size_t data_size = sizeof(BigArrayItem)*ARRAY_ITEMS_COUNT;

    /*If can use bitonic sort*/
    if ( test_sse41_CPU() ){
        WRITE_LOG(LOG_ERR, "allocate bitonic sort memories\n");
        /*we needed an extra buf to use with bitonic sort*/
        BigArrayPtr extra_buf = NULL;

        /*allocated memory should be aligned*/
        extra_buf = aligned_malloc(data_size, SSE2_ALIGNMENT);
        if ( !extra_buf ) {
            WRITE_LOG(LOG_ERR, "Can't allocate memories\n");
            abort();
        }
        sorted_array = aligned_malloc(data_size, SSE2_ALIGNMENT);
        if (!sorted_array) {
            WRITE_LOG(LOG_ERR, "Can't allocate memories\n");
            abort();
        }
        /*Read source data from STDIN*/
        struct UserChannel *channel = chan_if->Channel(chan_if, EInputOutputNode, 1, EChannelModeRead );
        const ssize_t readed = read( channel->fd, (void*)sorted_array, data_size);
        WRITE_FMT_LOG(LOG_ERR, "readed input file, expected size=%d, read size=%d\n", data_size, readed);
        assert(readed == data_size );
        WRITE_LOG(LOG_DETAILED_UI, "Start bitonic sorting\n");
        bitonic_sort_chunked((float*)sorted_array, ARRAY_ITEMS_COUNT, (float*)extra_buf, DEFAULT_CHUNK_SIZE);
        WRITE_LOG(LOG_DETAILED_UI, "Bitonic sorting complete\n");
        aligned_free(extra_buf);
    }
    /*If can't use bitonic  - then use c qsort*/
    else{
        WRITE_LOG(LOG_ERR, "qsort will used\n");
        BigArrayPtr unsorted_array = NULL;
        unsorted_array = malloc( data_size );
        if ( !unsorted_array ) {
            WRITE_LOG(LOG_ERR, "Can't allocate memories\n");
            abort();
        }
        if ( unsorted_array ){
            /*Read source data from STDIN*/
            const ssize_t readed = read( STDIN, (void*)unsorted_array, data_size);
            WRITE_FMT_LOG(LOG_ERR, "readed input file, expected size=%d, read size=%d\n", data_size, readed);
            assert(readed == data_size );
        }
        WRITE_LOG(LOG_DETAILED_UI, "Start qsort sorting\n");
        sorted_array = alloc_copy_array( unsorted_array, ARRAY_ITEMS_COUNT );
        qsort( sorted_array, ARRAY_ITEMS_COUNT, sizeof(BigArrayItem), quicksort_BigArrayItem_comparator );
        free(unsorted_array);
    }

    struct UserChannel *channel = chan_if->Channel( chan_if, EManagerNode, 1, EChannelModeWrite );
    assert(channel);
    channel->DebugPrint(channel, stderr);
    /*send crc of sorted array to the manager node*/
    uint32_t crc = array_crc( sorted_array, ARRAY_ITEMS_COUNT );
    WRITE_FMT_LOG(LOG_DEBUG, "write crc=%u into fd=%d", crc, channel->fd);
    write_crc( channel->fd, crc );
    /*send of crc complete*/

    /*prepare histogram data, with step defined by BASE_HISTOGRAM_STEP*/
    int histogram_len = 0;
    HistogramArrayPtr histogram_array = alloc_histogram_array_get_len(
            sorted_array, 0, ARRAY_ITEMS_COUNT, BASE_HISTOGRAM_STEP, &histogram_len );

    struct Histogram single_histogram;
    single_histogram.src_nodeid = nodeid;
    single_histogram.array_len = histogram_len;
    single_histogram.array = histogram_array;

    /*send histogram to manager*/
    channel = chan_if->Channel(chan_if, EManagerNode, 1, EChannelModeWrite);
    assert(channel);
    write_histogram( channel->fd, &single_histogram );

    struct UserChannel *chanw = chan_if->Channel(chan_if, EManagerNode, 1, EChannelModeWrite);
    channel = chan_if->Channel(chan_if, EManagerNode, 1, EChannelModeRead);
    assert(channel);
    assert(chanw);
    /*read request for detailed histogram and send it to manager*/
    read_requests_write_detailed_histograms( channel->fd, chanw->fd, nodeid, sorted_array, ARRAY_ITEMS_COUNT );
    WRITE_LOG(LOG_UI, "\n!!!!!!!Histograms Sending complete!!!!!!.\n");

    int *src_nodes_list = NULL;
    int src_nodes_count = chan_if->GetNodesListByType(chan_if, ESourceNode, &src_nodes_list );

    /*read range request (data start, end, dest node id) from manager node*/
    struct request_data_t req_data_array[src_nodes_count];
    init_request_data_array( req_data_array, src_nodes_count);
    channel = chan_if->Channel( chan_if, EManagerNode, 1, EChannelModeRead );
    assert(channel);
    read_range_request( channel->fd, req_data_array );

    /*sort request data by dest node id to be deterministic*/
    qsort( req_data_array, src_nodes_count, sizeof(struct request_data_t), quicksort_reqdata_by_destnodeid_comparator );

    /*send array data to the destination nodes, bounds for pieces of data was
     * received previously with range request */
    for ( int i=0; i < src_nodes_count; i++ ){
        int dst_nodeid = req_data_array[i].dst_nodeid;
        channel = chan_if->Channel( chan_if, EDestinationNode, dst_nodeid, EChannelModeWrite );
        int dst_write_fd = channel->fd;
        WRITE_FMT_LOG(LOG_DEBUG, "write_sorted_ranges write fd=%d", dst_write_fd );
        WRITE_FMT_LOG(LOG_DEBUG, "req_data_array[i].dst_nodeid=%d", req_data_array[i].dst_nodeid );
        write_sorted_ranges( dst_write_fd, &req_data_array[i], sorted_array );
    }
    WRITE_LOG(LOG_UI, "Sending Ranges Complete-OK");

    if ( test_sse41_CPU() )
        aligned_free(sorted_array);
    else
        free(sorted_array);
    return 0;
}


int main(int argc, char **argv){
    /* argv[0] is node name
     * expecting in format : "name-%d",
     * format for single node without decimal id: "name" */
    int ownnodeid= -1;
    int extracted_name_len=0;
    int res =0;
    WRITE_FMT_LOG(LOG_DEBUG, "Source node started argv[0]=%s.\n", argv[1] );

    /*get node type names via environnment*/
    char *source_node_type_text = getenv(ENV_SOURCE_NODE_NAME);
    char *dest_node_type_text = getenv(ENV_DEST_NODE_NAME);
    char *man_node_type_text = getenv(ENV_MAN_NODE_NAME);
    assert(source_node_type_text);
    assert(dest_node_type_text);
    assert(man_node_type_text);

    ownnodeid = ExtractNodeNameId( argv[1], &extracted_name_len );
    /*nodename should be the same we got via environment and extracted from argv[0]*/
    assert( !strncmp(source_node_type_text, argv[1], extracted_name_len ) );
    if ( ownnodeid == -1 ) ownnodeid=1; /*node id not specified for single node by default assign nodeid=1*/

    /*setup channels conf, now used static data but should be replaced by data from zrt*/
    struct ChannelsConfigInterface chan_if;
    SetupChannelsConfigInterface( &chan_if, ownnodeid, ESourceNode );

    /***********************************************************************
     Add channels configuration into config object */
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, IN_DIR, EChannelModeRead, EManagerNode, man_node_type_text );
    assert( res == 0 );
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, IN_DIR, EChannelModeRead, EDestinationNode, dest_node_type_text );
    assert( res == 0 );
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, OUT_DIR, EChannelModeWrite, EManagerNode, man_node_type_text );
    assert( res == 0 );
    res = AddAllChannelsRelatedToNodeTypeFromDir( &chan_if, OUT_DIR, EChannelModeWrite, EDestinationNode, dest_node_type_text );
    assert( res == 0 );
    /*add input channel into config*/
    res = chan_if.AddChannel( &chan_if, EInputOutputNode, 1, STDIN, EChannelModeRead ) != NULL? 0: -1;
    assert( res == 0 );
    /*--------------*/

    res = start_node(&chan_if, ownnodeid);

    CloseChannels(&chan_if);
    return res;
}

