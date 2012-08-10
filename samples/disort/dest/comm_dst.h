/*
 * comm_src.h
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */

#ifndef COMM_SRC_H_
#define COMM_SRC_H_

#include "distr_common.h"

struct ChannelsConfigInterface;

void
repreq_read_sorted_ranges( struct ChannelsConfigInterface *chan_if, BigArrayPtr dst_array, int dst_array_len );

void
write_sort_result( int fdw, int nodeid, BigArrayPtr sorted_array, int len );

#endif /* COMM_SRC_H_ */
