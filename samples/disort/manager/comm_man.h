/*
 * comm_man.h
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */

#ifndef COMM_MAN_H_
#define COMM_MAN_H_

#include "distr_common.h"

struct ChannelsConfigInterface;

void
read_crcs(struct ChannelsConfigInterface *chan_if, int *src_nodes_list, uint32_t *crc_array, int array_len);

void
recv_histograms( struct ChannelsConfigInterface *chan_if, int *src_nodes_list,
        struct Histogram *histograms, int wait_histograms_count );

/*@param complete Flag 0 say to client in request that would be requested again, 1-last request send
 *return Histogram Caller is responsive to free memory after using result*/
struct Histogram*
reqrep_detailed_histograms_alloc( int fdw, int fdr, int nodeid,
		const struct request_data_t* request_data, int complete );

void
write_range_request( int fdw, struct request_data_t** range, int len, int i );

struct sort_result*
read_sort_result( struct ChannelsConfigInterface *chan_if );

void
print_request_data_array( struct request_data_t* const range, int len );

#endif /* COMM_MAN_H_ */
