/*
 * map_reduce_lib.h
 *
 *      Author: yaroslav
 */

#ifndef __MAP_REDUCE_LIB_H__
#define __MAP_REDUCE_LIB_H__

#include "map_reduce_datatypes.h"

//forward decl
struct ChannelsConfigInterface;

enum { EMapNode=1, EReduceNode=2, EInputOutputNode=3 };

#define MIN(a,b) (a < b ? a : b )

#define SEND_BUFFER_SIZE             0x200000 //2MB
#define DEFAULT_MAP_CHUNK_SIZE_BYTES 0x100000 //1MB
#define MAP_CHUNK_SIZE_ENV           "MAP_CHUNK_SIZE"

/*Init MapReduceUserIf existing pointer object and get it ready to use
 comparator_f - if user provides NULL then default comparator will used */
#define PREPARE_MAPREDUCE(mif_p, map_f, combine_f, reduce_f, hashcomparator_f, \
			  hashstr_f, val_addr_is_data, item_size, h_size ) \
    (mif_p)->Map = (map_f);         /*set user Map function*/		\
    (mif_p)->Combine = (combine_f); /*set user Combine function */	\
    (mif_p)->Reduce = (reduce_f);   /*set user Reduce function */		\
    (mif_p)->HashComparator = (hashcomparator_f);				\
    /*set user function convert hash to a string */			\
    (mif_p)->HashAsString = (hashstr_f);					\
    (mif_p)->data.value_addr_is_data = (val_addr_is_data);		\
    (mif_p)->data.mr_item_size = item_size;				\
    (mif_p)->data.hash_size = (h_size);


struct MapReduceUserIf{
    /* read input buffer, allocate and fill keys & values arrays.
     * User is not responsible to free keys & values arrays;
     * @param last_chunk 1 if last chunk data provided, otherwise 0
     * @param mapped Result mapped hashes,keys and data, user is not 
     * responsible to free it is
     * @return handled data pos*/
    int (*Map)(const char *data, 
	       size_t size, 
	       int last_chunk, 
	       Buffer *map_buffer );
    /* Waiting sorted data by keys.
     * reduce and put reduced into reduced_keys, reduced_values*/
    int (*Combine)( const Buffer *map_buffer, 
		    Buffer *reduce_buffer );
    /*reduce and output data into stdout*/
    int (*Reduce)( const Buffer *reduce_buffer );
    /*comparator can be overrided by user, otherwise library will use own*/
    int (*HashComparator)(const void *p1, const void *p2);
    /*function converts hash to a string can be overrided by user, otherwise library 
      will use own. It's function used by library for test purposes*/
    char* (*HashAsString)( char* str, const uint8_t* hash, int size);
    /*data*/
    struct MapReduceData data;
};


int MapNodeMain(struct MapReduceUserIf *userif, 
		struct ChannelsConfigInterface *ch_if );

int ReduceNodeMain(struct MapReduceUserIf *userif, 
		   struct ChannelsConfigInterface *ch_if );


/*Functions related to implementation, moved into header to be tested in separate main*/

/*Read MR items from map buffer and pass every "step" item into histogram->buffer array
 *histogram created histogram
 *@return count of added items into histogram*/
size_t
GetHistogram( struct MapReduceUserIf *mif,
	      const Buffer* map,
	      int step, 
	      Histogram *histogram );

/*Create resulted hash list, every hash in list is mean maximum hash value that 
 *can be sent to reducer node which have the same index as item in array. Last divider
 *item value must be maximum as possible, for example 0xffffff.
 *@param histograms array of histograms
 *@param histograms_count histograms in array
 *@param divider_array Result of histograms summarization*/
void 
GetReducersDividerArrayBasedOnSummarizedHistograms( struct MapReduceUserIf *mif,
						    Histogram *histograms, 
						    int histograms_count, 
						    Buffer *divider_array );

#endif //__MAP_REDUCE_LIB_H__


