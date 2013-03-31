#ifndef __MAP_REDUCE_DATATYPES_H__
#define __MAP_REDUCE_DATATYPES_H__

#include <stddef.h> //size_t

#include "elastic_mr_item.h"
#include "buffer.h"

/*forward decl*/
struct ChannelsConfigInterface;

/*Histogram - slice of data, where every N-item "step_hist_common" added into
 *histogram buffer. Hash of key is used as histogram item. 
 *"step_hist_last" last M-item added into histogram.*/
typedef Buffer HashBuffer;
typedef struct Histogram{
    int        src_nodeid;
    uint16_t   step_hist_common;
    uint16_t   step_hist_last;
    HashBuffer buffer;
} Histogram;


struct MapReduceData{
    int mr_item_size;     /*user must provide right mr item structure size*/
    int hash_size;        /*set BufItemElastic::key_hash size*/
    int value_is_data;    /*use BufItemElastic::addr as data*/
    //internals
    Histogram *histograms_list;
    int        histograms_count; /*histograms count is equal to map nodes count*/
    HashBuffer dividers_list; /*divider list is used to divide map data to reducers*/
};


/*currently pointer functions not used, but real function exist in 'internals' section*/
struct MapNodeEvents{
    /*@return actual data size copied into input_buffer from input(file|stdin) */
    size_t
    (*MapInputDataProvider)( int fd, 
			     char **input_buffer, 
			     size_t requested_buf_size, 
			     int unhandled_data_pos );
    /* @param buf input  buffer
     * @param buf_size  buffer size
     * @param result_keys Saving orocessed keys. should be valid pointer.
     * * @param result_values Save processed values. should be valid pointer.
     * @return unhandled data buffer pos*/
    size_t
    (*MapInputDataLocalProcessing)( const char *buf, 
				    size_t buf_size, 
				    int last_chunk, 
				    Buffer *result );

    /* read key_hash'es from map buffer and create histogram
     * For first call every map node should create and send histogram to all
     * another map nodes, to get resulted summarized histogram. Based on it
     * we get a dividers list which helps to distribute data on Reducers
     * @param map sorted map data*/
    void
    (*MapCreateHistogramSendEachToOtherCreateDividersList)
    ( struct ChannelsConfigInterface *ch_if, 
      struct MapReduceData *data, 
      const Buffer *map );
    /**/
    void
    (*MapSendToAllReducers)(struct ChannelsConfigInterface *ch_if, 
			    int last_data, 
			    const Buffer *map);
};

#endif //__MAP_REDUCE_DATATYPES_H__
