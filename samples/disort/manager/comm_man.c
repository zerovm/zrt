/*
 * communic.c
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "distr_common.h"
#include "defines.h"
#include "comm.h"
#include "channels_conf.h"

/*reading crc from every source node
 *READ 1x uint32_t, crc*/
void
read_crcs(struct ChannelsConfigInterface *chan_if, int *src_nodes_list, uint32_t *crc_array, int array_len){
	WRITE_FMT_LOG(LOG_DEBUG, "Reading read_crcs %d count\n", array_len);
	for( int i=0; i < array_len; i++ ){
		struct UserChannel *channel = chan_if->Channel(chan_if, ESourceNode, src_nodes_list[i], EChannelModeRead );
		assert(channel);
		channel->DebugPrint(channel, stderr);
		WRITE_FMT_LOG(LOG_DEBUG, "Reading fd=%d, crc #%d; ", channel->fd, i);
		read_channel(channel->fd, (char*) &crc_array[i], sizeof(uint32_t) );
		WRITE_FMT_LOG(LOG_DEBUG, "Read OK crc=%u\n", crc_array[i]);
	}
	WRITE_LOG(LOG_DEBUG, "crc read ok");
}


/*READ 1x struct packet_data_t
 *READ array_len x  HistogramArrayItem, array*/
void
recv_histograms( struct ChannelsConfigInterface *chan_if, int *src_nodes_list,
        struct Histogram *histograms, int wait_histograms_count )
{
	WRITE_FMT_LOG(LOG_DEBUG, "Reading histograms %d count\n", wait_histograms_count);
	for( int i=0; i < wait_histograms_count; i++ ){
	    struct UserChannel *channel = chan_if->Channel(chan_if, ESourceNode, src_nodes_list[i], EChannelModeRead);
	    assert(channel);
	    channel->DebugPrint(channel, stderr);
		WRITE_FMT_LOG(LOG_DEBUG,"Reading fd=%d, histogram #%d\n", channel->fd, i);
		struct packet_data_t t; t.type = EPACKET_UNKNOWN;
		read_channel(channel->fd, (char*) &t, sizeof(t) );
		WRITE_FMT_LOG(LOG_DEBUG,"readed packet: type=%d, size=%d, src_nodeid=%d\n", t.type, (int)t.size, t.src_nodeid );
		struct Histogram *histogram = &histograms[i];
		if ( EPACKET_HISTOGRAM == t.type ){
			histogram->array = malloc( t.size );
			read_channel(channel->fd, (char*) histogram->array, t.size);
			histogram->array_len = t.size / sizeof(HistogramArrayItem);
			histogram->src_nodeid = t.src_nodeid;
		}
		else{
			WRITE_FMT_LOG(LOG_ERR, "recv_histograms::wrong packet type %d size %d\n", t.type, (int)t.size);
			exit(-1);
		}
	}
	WRITE_LOG(LOG_DEBUG, "Hitograms ok");
}


/*i/o 2 synchronous files
 *WRITE  1x struct request_data_t
 *READ 1x char, reply
 *WRITE  1x char is_complete flag, if 0
 *READ 1x int, nodeid
 *WRITE  1x char, reply
 *READ 1x size_t, array_len
 *WRITE  1x char, reply
 *READ array_size, array HistogramArrayItem
 * @param complete Flag 0 say to client in request that would be requested again, 1-last request send
 *return Histogram Caller is responsive to free memory after using result*/
struct Histogram*
reqrep_detailed_histograms_alloc( int fdw, int fdr, int nodeid,
		const struct request_data_t* request_data, int complete ){
    WRITE_FMT_LOG(LOG_DEBUG, "put request [ dstnodeid=%d, f_item_index=%d, l_item_index=%d ]\n",
            request_data->dst_nodeid, request_data->first_item_index, request_data->last_item_index );
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] complete=%d, Write detailed histogram requests to %d\n", nodeid, complete, fdw );
	//send detailed histogram request
	write_channel(fdw, (const char*)request_data, sizeof(struct request_data_t));
	write_channel(fdw, (const char*)&complete, sizeof(complete));
	WRITE_LOG(LOG_DEBUG, "detailed histograms receiving");
	//recv reply
	struct Histogram *item = malloc(sizeof(struct Histogram));
	read_channel(fdr, (char*) &item->src_nodeid, sizeof(item->src_nodeid));
	read_channel(fdr, (char*) &item->array_len, sizeof(item->array_len));
	size_t array_size = sizeof(HistogramArrayItem)*item->array_len;
	item->array = malloc(array_size);
	read_channel(fdr, (char*) item->array, array_size);

	if ( item->array_len ){
        WRITE_FMT_LOG(LOG_DEBUG, "get response [ srcnodeid=%d, histograms count=%d, first_item_index=%d ]\n",
                    item->src_nodeid, item->array_len, item->array->item_index );
	}
	WRITE_FMT_LOG(LOG_DEBUG, "detailed histograms received from%d: expected len:%d\n",
			item->src_nodeid, (int)(sizeof(HistogramArrayItem)*item->array_len) );
	return item;
}

/*WRITE 1x struct packet_data_t
 *WRITE 1x struct request_data_t*/
void
write_range_request( int fdw, struct request_data_t** range, int len, int i ){
	struct packet_data_t t;
	t.type = EPACKET_SEQUENCE_REQUEST;
	t.src_nodeid = (int)range[i][0].dst_nodeid;
	t.size = len;
	WRITE_FMT_LOG(LOG_DEBUG, "t.ize=%d, t.type=%d(EPACKET_SEQUENCE_REQUEST)\n", (int)t.size, t.type);
	write_channel(fdw, (const char*)&t, sizeof(t));
	for ( int j=0; j < len; j++ ){
		/*request data copy*/
		struct request_data_t data = range[j][i];
		data.dst_nodeid = j+1;
		write_channel(fdw, (const char*)&data, sizeof(struct request_data_t));
		WRITE_FMT_LOG(LOG_DEBUG, "write_range_request %d %d %d %d\n",
				data.dst_nodeid, data.src_nodeid, data.first_item_index, data.last_item_index );
	}
	WRITE_LOG(LOG_DEBUG, "write_range_request sending complete\n" );
}

/*
 *READ waiting_results x struct sort_result*/
struct sort_result*
read_sort_result( struct ChannelsConfigInterface *chan_if ){
    int *src_nodes_list = NULL;
    int src_nodes_count = chan_if->GetNodesListByType( chan_if, ESourceNode, &src_nodes_list );
	struct sort_result *results = malloc( src_nodes_count*sizeof(struct sort_result) );
	for ( int i=0; i < src_nodes_count; i++ ){
	    struct UserChannel *channel = chan_if->Channel(chan_if, ESourceNode, src_nodes_list[i], EChannelModeRead );
		read_channel( channel->fd, (char*) &results[i], sizeof(struct sort_result) );
	}
	return results;
}

void
print_request_data_array( struct request_data_t* const range, int len ){
	for ( int j=0; j < len; j++ )
	{
		WRITE_FMT_LOG(LOG_DEBUG, "SEQUENCE N:%d, dst_pid=%d, src_pid=%d, findex %d, lindex %d \n",
				j, range[j].dst_nodeid, range[j].src_nodeid,
				range[j].first_item_index, range[j].last_item_index );
	}
}

