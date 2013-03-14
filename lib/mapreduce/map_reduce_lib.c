
#include <stdlib.h> //calloc
#include <stdio.h>  //puts
#include <string.h> //memcpy
#include <unistd.h> //ssize_t
#include <assert.h> //assert
#include <sys/types.h> //temp read file
#include <sys/stat.h> //temp read file
#include <fcntl.h> //temp read file

#include "map_reduce_lib.h"
#include "eachtoother_comm.h"
#include "channels_conf.h"
#include "mr_defines.h"

#define MAX_UINT32 4294967295
#define MAP_NODE_NO_EXCLUDE 0
#define MAP_NODE_EXCLUDE 1

struct MapReduceUserIf *__userif = NULL;


#ifdef DEBUG
void PrintBuffers( const Buffer *keys, const Buffer *values ){
    if ( !keys->data || !values->data ) return;
    uint32_t key;
    uint32_t value;
    for (int i=0; i < keys->header.count; i++){
	GetBufferItem(keys, i, &key);
	GetBufferItem(values, i, &value);
	//WRITE_FMT_LOG( "%u = %u \n", key, value );
    }
    fflush(0);
}

#endif //DEBUG

/*****************************************************************************
 * local sort of mapped keys and values*/

static int KeyComparatorUint32(const void *p1, const void *p2){
    const uint32_t *v1 = (uint32_t*)p1;
    const uint32_t *v2 = (uint32_t*)p2;
    if ( *v1 < *v2 ) return -1;
    else if ( *v1 > *v2 ) return 1;
    else return 0;
}



size_t MapInputDataProvider( int fd, char **input_buffer, size_t requested_buf_size, int unhandled_data_pos ){
    WRITE_FMT_LOG("MapInputDataProvider *input_buffer=%p, fd=%d, requested_buf_size=%u, unhandled_data_pos=%d\n",
		  *input_buffer, fd, (uint32_t)requested_buf_size, unhandled_data_pos );
    size_t rest_data_in_buffer = requested_buf_size - unhandled_data_pos;
    int first_call = *input_buffer == NULL ? 1 : 0;
    if ( !*input_buffer ){
	/*if input buffer not yet initialized then alloc it. It should be deleted by our framework after use*/
	*input_buffer = calloc( requested_buf_size+1, sizeof(char) );
	/*no data in buffer, it seems to be a first call of DataProvider*/
	rest_data_in_buffer = 0;
    }

    /**********************************************************************
     * Check for unsupported data handling. User should get handled data pos,
     * and we should concatenate unprocessed data with new data from input file.
     * For first launch of DataProvider unhandled_data_pos is ignoring*/
    if ( !first_call ){
	if ( !unhandled_data_pos ){
	    /*user Map function not hadnled data at all.
	     * If case then data amount is too little and user can not handle it and returning 0
	     * MapReduce library should be fix it as a bug*/
	    assert( unhandled_data_pos );
	}
	else if ( unhandled_data_pos < requested_buf_size/2 ){
	    /* user Map function hadnled less than half of data. In normal case we need to move the rest
	     * of data to begining of buffer, but we can't do it just move because end of moved data and
	     * start of unhandled data is overlaps;
	     * For a while we don't want create another heap buffers to join rest data and new data; */
	    assert( unhandled_data_pos > requested_buf_size/2 );
	}
	else if ( unhandled_data_pos != requested_buf_size )
	    {
		/*if not all data was processed in previous call of DataProvider then copy the rest into input_buffer*/
		memcpy( (*input_buffer), (*input_buffer)+unhandled_data_pos, rest_data_in_buffer );
	    }
    }
    ssize_t readed = read( fd, *input_buffer, requested_buf_size - rest_data_in_buffer );

    WRITE_FMT_LOG("MapInputDataProvider OK return=%u\n", (uint32_t)(rest_data_in_buffer+readed) );
    return rest_data_in_buffer+readed;
}


void LocalSort(const Buffer *keys, const Buffer *values, Buffer *sorted_keys, Buffer *sorted_values){
    /*read keys buffer and create sortable array*/
    struct 	SortableKeyVal *sort_array = malloc( sizeof(struct SortableKeyVal) * keys->header.count );
    for ( int i=0; i < keys->header.count; i++ ){
	GetBufferItem(keys, i, &sort_array[i].key);
	sort_array[i].original_index = i;
    }
    /*sort created array*/
    qsort( sort_array, keys->header.count, sizeof(*sort_array), KeyComparatorUint32 );

    /*alloc memories for sorted buffers*/
    assert( !AllocBuffer( sorted_keys, keys->header.data_type, keys->header.count ) );
    assert( !AllocBuffer( sorted_values, values->header.data_type, values->header.count ) );

    uint32_t key;
    uint32_t value;
    /*copy changes from original buffers into sorted_keys, sorted_values buffers based on sorted array with indexes*/
    for ( int i=0; i < keys->header.count; i++ ){
	/*copy keys & values*/
	GetBufferItem( keys, sort_array[i].original_index, &key );
	SetBufferItem( sorted_keys, i, &key );
	GetBufferItem( values, sort_array[i].original_index, &value );
	SetBufferItem( sorted_values, i, &value );
    }
    sorted_keys->header.count = sorted_values->header.count = keys->header.count;
    /*don't need sort_array*/
    free(sort_array);
}


size_t
AllocHistogram(
	       const KeyType *array, const int array_len, int step, Histogram *histogram ){
    memset(histogram, '\0', sizeof(*histogram));
    if ( !array_len ) return 0;
    if ( !step ){
	histogram->array_len = 1;
	histogram->array = calloc( histogram->array_len, sizeof(KeyType) );
	histogram->array[0] = array[array_len-1];
	histogram->step_hist_common = histogram->step_hist_last = array_len;
    }
    else{
	histogram->array = calloc( array_len/step + 1, sizeof(KeyType) );
	histogram->step_hist_last = histogram->step_hist_common = step;
	int i = -1; /*array index*/
	do{
	    histogram->step_hist_last = MIN( step, array_len-i-1 );
	    i += histogram->step_hist_last;
	    histogram->array[ histogram->array_len++ ] = array[i];
	}while( i < array_len-1 );

    }
    return histogram->array_len;
}

/************************************************************************
 * EachToOtherPattern callback.
 * Every map node read histograms from another nodes*/
void ReadHistogramFromNode( struct EachToOtherPattern *p_this, int nodetype, int index, int fdr ){
    WRITE_FMT_LOG( "eachtoother:read( from:index=%d, from:fdr=%d)\n", index, fdr );
    /*read histogram data from another nodes*/
    struct MapReduceData *mapred_data = (struct MapReduceData *)p_this->data;
    /*histogram has always the same index as node in nodes_list reading from*/
    assert( index < mapred_data->histograms_count );
    Histogram *histogram = &mapred_data->histograms_list[index];
    WRITE_FMT_LOG("ReadHistogramFromNode index=%d\n", index);
    assert( !histogram->array ); 	/*before reading histogram array pointer should be empty*/
    read(fdr, histogram, sizeof(Histogram) );
    if ( histogram->array_len ){
	histogram->array = calloc( histogram->array_len, sizeof(*histogram->array) );
	read(fdr, histogram->array, histogram->array_len * sizeof(*histogram->array) );
    }
}

/************************************************************************
 * EachToOtherPattern callbacks.
 * Every map node write histograms to another nodes*/
void WriteHistogramToNode( struct EachToOtherPattern *p_this, int nodetype, int index, int fdw ){
    WRITE_FMT_LOG( "eachtoother:write( to:index=%d, to:fdw=%d)\n", index, fdw );
    /*write own data histogram to another nodes*/
    struct MapReduceData *mapred_data = (struct MapReduceData *)p_this->data;
    /*nodeid in range from 1 up to count*/
    int own_histogram_index = p_this->conf->ownnodeid-1;
    assert( own_histogram_index < mapred_data->histograms_count );
    Histogram *histogram = &mapred_data->histograms_list[own_histogram_index];
    WRITE_FMT_LOG("WriteHistogramToNode index=%d\n", index);
    /*write Histogram struct, array contains pointer it should be ignored by receiver */
    write(fdw, histogram, sizeof(Histogram));
    if ( histogram->array ){
	/*write histogram data*/
	write(fdw, histogram->array, histogram->array_len*sizeof(*histogram->array));
    }
}


/*******************************************************************************
 * Summarize Histograms, shrink histogram and get as dividers array*/
void SummarizeHistograms( Histogram *histograms, int histograms_count, int dividers_count, KeyType *divider_array ){
    assert(divider_array);
    /*dividers_count is the same as reduce nodes count, it is can't be equal to 0*/
    assert(dividers_count != 0);
    memset( divider_array, '\0', sizeof(*divider_array)*dividers_count );
    /*calculate all histograms size*/
    size_t size_all_histograms_data = 0;
    for ( int i=0; i < histograms_count; i++ ){
	/*if histogram has more one item*/
	if ( histograms[i].array_len > 1 ){
	    size_all_histograms_data += histograms[i].step_hist_common * (histograms[i].array_len -1);
	    size_all_histograms_data += histograms[i].step_hist_last;
	}
	else if ( histograms[i].array_len == 1 ){
	    assert( histograms[i].step_hist_last == histograms[i].step_hist_common );
	    size_all_histograms_data += histograms[i].step_hist_last;
	}
    }
    WRITE_FMT_LOG("all histogram size=%u\n", (uint32_t)size_all_histograms_data);
    /*calculate block size divider for single reducer*/
    size_t generic_divider_block_size = size_all_histograms_data / dividers_count;
    size_t current_divider_block_size = 0;

    uint32_t minimal = 0;
    /*start histograms processing*/
    int indexes[dividers_count];
    memset( &indexes, '\0', sizeof(indexes) );
    int current_divider_index = 0;
    int histogram_index_with_minimal_value = 0;
    do{
	/*found minimal value among currently indexed histogram values*/
	minimal = 0;
	for ( int i=0; i < histograms_count; i++ ){ /*loop for histograms*/
	    /*check bounds of current histogram*/
	    if ( indexes[i] < histograms[i].array_len ){
		if ( minimal == 0  )
		    minimal = histograms[i].array[ indexes[i] ];
		/*get minimal value among currently indexed histogram values*/
		if ( histograms[i].array[ indexes[i] ] <= minimal ){
		    histogram_index_with_minimal_value = i;
		    minimal = histograms[i].array[ indexes[i] ];
		}
	    }
	}

	/*add found divider value to current divider*/
	divider_array[current_divider_index] = minimal;

	/*increase current divider block size*/
	if ( indexes[histogram_index_with_minimal_value] < histograms[histogram_index_with_minimal_value].array_len-1 ){
	    current_divider_block_size += histograms[histogram_index_with_minimal_value].step_hist_common;
	}
	else{
	    current_divider_block_size += histograms[histogram_index_with_minimal_value].step_hist_last;
	}

	/*one divider filling complete*/
	if ( current_divider_block_size >= generic_divider_block_size ){
	    WRITE_FMT_LOG( "%d item= %u ", indexes[histogram_index_with_minimal_value], minimal ); fflush(0);
	    current_divider_block_size=0;
	    current_divider_index++;
	}
	indexes[histogram_index_with_minimal_value]++;
    }while( current_divider_index < dividers_count-1 ); /*while pre last divider processing*/

    /*For last divider just set max uint value, to send all rest values into last divider*/
    divider_array[dividers_count-1] = MAX_UINT32;
#ifdef DEBUG
    WRITE_LOG("\ndivider_array=[");
    for( int i=0; i < dividers_count ; i++ ){
	WRITE_FMT_LOG(" %u", divider_array[i]);
    }
    WRITE_LOG("]\n");
#endif
}


size_t MapInputDataLocalProcessing( const char *buf, size_t buf_size, int last_chunk, Buffer*result_keys, Buffer *result_values ){
    size_t unhandled_data_pos = 0;
    WRITE_FMT_LOG("sbrk()=%p\n", sbrk(0) );
    WRITE_FMT_LOG("======= new portion of data read: input buffer=%p, buf_size=%u\n", buf, (uint32_t)buf_size );
    /*Setup keys, values buffer types. user required to allocate buffers spaces (Buffer::data)*/
    Buffer keys;
    Buffer values;
    /*user Map process input data and allocate keys, values buffers*/
    unhandled_data_pos = __userif->Map( buf, buf_size, last_chunk, &keys, &values );
    WRITE_FMT_LOG("User Map() function result : keys_count=%u, values_count=%u, unhandled pos=%u\n",
		  (uint32_t)keys.header.count, (uint32_t)values.header.count, (uint32_t)unhandled_data_pos );
    assert( keys.header.count == values.header.count );

    WRITE_LOG("1\n");
#ifdef DEBUG
    PrintBuffers( &keys, &values );
#endif //DEBUG
    WRITE_LOG("2\n");
    Buffer sorted_keys;
    Buffer sorted_values;
    LocalSort( &keys, &values, &sorted_keys, &sorted_values);
    WRITE_LOG("3\n");

    FreeBufferData(&keys);
    WRITE_LOG("4\n");
    FreeBufferData(&values);

    WRITE_FMT_LOG("MapCallEvent:sorted map, count=%u\n", (uint32_t)sorted_keys.header.count);
#ifdef DEBUG
    PrintBuffers( &sorted_keys, &sorted_values );
#endif //DEBUG

    if ( __userif->Combine ){
	__userif->Combine( &sorted_keys, &sorted_values, result_keys, result_values );
    }
    else{
	//use just sorted key-value data, because user does not defined Combine function
	*result_keys = sorted_keys;
	*result_values = sorted_values;
	sorted_keys.data = NULL;
	sorted_values.data = NULL;
    }

    FreeBufferData(&sorted_keys);
    FreeBufferData(&sorted_values);

    WRITE_FMT_LOG("MapCallEvent: combined keys, count=%u\n", (uint32_t)result_keys->header.count);
#ifdef DEBUG
    PrintBuffers( result_keys, result_values );
#endif //DEBUG
    return unhandled_data_pos;
}


void MapCreateHistogramSendEachToOtherCreateDividersList(
							 struct ChannelsConfigInterface *ch_if, struct MapReduceData *data, const Buffer *input_keys ){
    /*For first call need to create histogram and send to all map nodes
     * To finally get dividers list which helps distribute of data to Reducers*/

    /*retrieve Map/Reducer nodes list and nodes count*/
    int *map_nodes_list = NULL;
    int map_nodes_count = ch_if->GetNodesListByType( ch_if, EMapNode, &map_nodes_list );

    /*get reducers count, this same as dividers count for us, reducers list is not needed now*/
    int *reduce_nodes_list = NULL;
    int reduce_nodes_count = 
	ch_if->GetNodesListByType( ch_if, EReduceNode, &reduce_nodes_list );

    //generate histogram with offset
    int current_node_index = ch_if->ownnodeid-1;
    assert(map_nodes_count); /*it should not be zero*/
    size_t hist_step = input_keys->header.count / 100 / map_nodes_count;
    /*save histogram for own data always into current_node_index-pos of histograms array*/
    AllocHistogram( (const KeyType*)input_keys->data, input_keys->header.count, hist_step,
		    &data->histograms_list[current_node_index] );
#ifdef DEBUG
    WRITE_LOG("\nHistogram=[");
    for( int i=0; i < data->histograms_list[current_node_index].array_len ; i++ ){
	WRITE_FMT_LOG(" %u", data->histograms_list[current_node_index].array[i]);
    }
    WRITE_LOG("]\n"); fflush(0);
#endif

    WRITE_FMT_LOG("MapCallEvent: histogram created. count=%d, fstep=%u, lstep=%u\n",
		  (int)data->histograms_list[current_node_index].array_len,
		  (uint16_t)data->histograms_list[current_node_index].step_hist_common,
		  (uint16_t)data->histograms_list[current_node_index].step_hist_last );

    /*use eachtoother pattern to map nodes communication*/
    struct EachToOtherPattern histogram_from_all_to_all_pattern = {
	.conf = ch_if,
	.data = data,
	.Read = ReadHistogramFromNode,
	.Write = WriteHistogramToNode
    };
    StartEachToOtherCommunication( &histogram_from_all_to_all_pattern, EMapNode );

    /*now every map node contains histograms from all map nodes, summarize histograms,
     * to get distribution of all data. Result of summarization write into divider_array*/
    data->dividers_count = reduce_nodes_count;
    data->dividers_list = calloc( reduce_nodes_count, sizeof(*__userif->data.dividers_list) );
    SummarizeHistograms(
			data->histograms_list,
			data->histograms_count,
			data->dividers_count,
			data->dividers_list );

    free(map_nodes_list);
    free(reduce_nodes_list);
}


void WriteDataToReduce(int fdw, const Buffer *keys, const Buffer *values, int data_start_index, int items_count,
		       int last_data_flag ){
    /*send to reducer with current_divider_index index*/
    struct BufferHeader divider_keys_header = {
	.buf_size = items_count * keys->header.item_size,
	.data_type = keys->header.data_type,
	.item_size = keys->header.item_size,
	.count = items_count
    };
    struct BufferHeader divider_values_header = {
	.buf_size = items_count * values->header.item_size,
	.data_type = values->header.data_type,
	.item_size = values->header.item_size,
	.count = items_count
    };

    ssize_t bytes = 0;
    /*write last data flag 0 | 1, if reducer receives 1 then it should exclude this map node from communications*/
    bytes = write( fdw, &last_data_flag, sizeof(last_data_flag) );
    WRITE_FMT_LOG( "write=%d / %d last_data_flag\n", (int)bytes, (int)sizeof(divider_keys_header) );
    /*write keys records size*/
    bytes = write(fdw, &divider_keys_header, sizeof(divider_keys_header));
    WRITE_FMT_LOG( "write=%d / %d\n", (int)bytes, (int)sizeof(divider_keys_header) );
    assert( sizeof(divider_keys_header) == bytes );
    /*write keys data */
    uint32_t temp;
    uint32_t temp2;
    GetBufferItem( keys, data_start_index, &temp );
    GetBufferItem( keys, data_start_index+items_count, &temp2 );
    ////////////////////////
    bytes = write(fdw, GetBufferPointer( keys, data_start_index ), divider_keys_header.buf_size );
    WRITE_FMT_LOG( "write=%d / %d, starting from %u to %u\n",
		   (int)bytes, (int)divider_keys_header.buf_size, temp, temp2 );
    assert( divider_keys_header.buf_size == bytes );
    /*write values records size*/
    bytes = write(fdw, &divider_values_header, sizeof(divider_values_header));
    WRITE_FMT_LOG( "write=%d / %d\n", (int)bytes, (int)sizeof(divider_values_header) );
    assert( sizeof(divider_values_header) == bytes );
    /*write values data */
    bytes = write(fdw, GetBufferPointer( values, data_start_index), divider_values_header.buf_size );
    WRITE_FMT_LOG( "write=%d / %d\n", (int)bytes, (int)divider_values_header.buf_size );
    assert( divider_values_header.buf_size == bytes );
}


void MapSendKeysValuesToAllReducers(struct ChannelsConfigInterface *ch_if, int last_data, Buffer *keys, Buffer *values){
    /*get reducer count, this is same as dividers count for us, reducers list is not needed now*/
    int *reduce_nodes_list = NULL;
    int dividers_count  = ch_if->GetNodesListByType( ch_if, EReduceNode, &reduce_nodes_list );

    /*send keys-values to Reducer nodes, using dividers data from data_start_index to
     * data_end_index range their max value less or equal to current divider value */
    int data_start_index = 0;
    int count_in_section = 0;
    KeyType current_key;
    int current_divider_index = 0;

    /*loop for data*/
    for ( int j=0; j < keys->header.count; j++ ){
	GetBufferItem( keys, j, &current_key);
	if ( current_key <= __userif->data.dividers_list[current_divider_index] ){
	    count_in_section++;
	}

	/*if last item or current key above than current divider*/
	if ( j+1 == keys->header.count ||
	     current_key > __userif->data.dividers_list[current_divider_index] ){
	    if ( j > 0 ){
		KeyType prev;
		GetBufferItem( keys, j-1, &prev);
		WRITE_FMT_LOG( "reducer #%d divider=%u prev item[%d]=%u\n",
			       reduce_nodes_list[current_divider_index], __userif->data.dividers_list[current_divider_index],
			       j-1, prev );
	    }
	    WRITE_FMT_LOG( "reducer #%d divider=%u new item[%d]=%u\n",
			   reduce_nodes_list[current_divider_index], __userif->data.dividers_list[current_divider_index],
			   j, current_key );

	    /*send to reducer with current_divider_index index*/
	    struct UserChannel *channel = ch_if->Channel(ch_if,
							 EReduceNode, reduce_nodes_list[current_divider_index], EChannelModeWrite);
	    assert(channel);
	    int fdw = channel->fd;
	    last_data = last_data ? MAP_NODE_EXCLUDE : MAP_NODE_NO_EXCLUDE;
	    WRITE_FMT_LOG( "fdw=%d to reducer node %d write %d items\n", fdw,
			   reduce_nodes_list[current_divider_index], (int)count_in_section );
	    WriteDataToReduce( fdw, keys, values, data_start_index, count_in_section, last_data );

	    /*set start data index of next divider*/
	    data_start_index = j+1;
	    /*reset count for next divider*/
	    count_in_section = 0;
	    /*switch to next divider, do increment if it's not last handling item*/
	    if ( j+1 < keys->header.count )
		{
		    current_divider_index++;
		}
	    assert( current_divider_index < dividers_count );
	}
    }

    /*if keys/values list is empty then nothing sent to reduce nodes previously.
     *But every map node should send at least one time to every reduce node last_data flag
     *and another data required by communication protocol*/
    if ( !keys->header.count ){
	for ( int i=0; i < dividers_count; i++ ){
            struct UserChannel *channel = ch_if->Channel(ch_if, EReduceNode, reduce_nodes_list[i], EChannelModeWrite);
            assert(channel);
	    int fdw = channel->fd;
	    WRITE_FMT_LOG( "fdw=%d to reducer node items_count=0, flag=MAP_NODE_EXCLUDE\n", fdw );
	    WriteDataToReduce( fdw, keys, values, 0, 0, MAP_NODE_EXCLUDE );
	}
    }
}

void InitMapInternals(struct MapReduceUserIf *userif, const struct ChannelsConfigInterface *chif, struct MapNodeEvents* ev){
    /*get histograms count for MapReduceData*/
    int *nodes_list_unwanted = NULL;
    userif->data.histograms_count = chif->GetNodesListByType(chif, EMapNode, &nodes_list_unwanted );
    free(nodes_list_unwanted);
    userif->data.histograms_list = calloc( userif->data.histograms_count, sizeof(*userif->data.histograms_list) );

    ev->MapCreateHistogramSendEachToOtherCreateDividersList = MapCreateHistogramSendEachToOtherCreateDividersList;
    ev->MapInputDataLocalProcessing = MapInputDataLocalProcessing;
    ev->MapInputDataProvider = MapInputDataProvider;
    ev->MapSendKeysValuesToAllReducers = MapSendKeysValuesToAllReducers;
}


int MapNodeMain(struct MapReduceUserIf *userif, struct ChannelsConfigInterface *chif ){
    WRITE_LOG("MapNodeMain\n");
    assert(userif);
    __userif = userif;

    struct MapNodeEvents events;
    InitMapInternals(userif, chif, &events);


    /*should be initialized at fist call of DataProvider*/
    char *buffer = NULL;
    size_t returned_buf_size = 0;
    /*default block size for input file*/
    size_t split_input_size=DEFAULT_MAP_CHUNK_SIZE_BYTES;

    /*get from environment block size for input file*/
    if ( getenv(MAP_CHUNK_SIZE_ENV) )
	split_input_size = atoi(getenv(MAP_CHUNK_SIZE_ENV));
    WRITE_FMT_LOG( "MAP_CHUNK_SIZE_BYTES=%d\n", split_input_size );
	
    /*by default can set any number, but actually it should be point to start of unhandled data,
     * for fully handled data it should be set to data size*/
    size_t current_unhandled_data_pos = 0;
    int last_chunk = 0;

    /*get input channel*/
    struct UserChannel *channel = chif->Channel(chif,EInputOutputNode, 1, EChannelModeRead);
    assert(channel);

    do{
	free(buffer), buffer=NULL;

	/*4rd parameter is not used for first call, for another calls it should be assigned by Map call return value*/
	returned_buf_size = events.MapInputDataProvider(
							channel->fd,
							&buffer,
							split_input_size,
							current_unhandled_data_pos
							);
	last_chunk = returned_buf_size < split_input_size? 1 : 0; //last chunk flag

	/*before using buffers should be initialized in user Map function, by AllocBuffer*/
	Buffer keys;
	Buffer values;

	if ( !returned_buf_size ){
	    /*if no input data then just set no results in uninitialized Buffers.
	     *anyway required to send to all maps empty histogram and receive histograms from another map nodes;*/
	    AllocBuffer( &keys, EUint32, 1 );
	    AllocBuffer( &values, EUint32, 1 );
	}
	else{
	    /*call users Map, Combine functions only for non empty data set*/
	    WRITE_FMT_LOG( "last_chunk=%d\n", last_chunk );
	    current_unhandled_data_pos = events.MapInputDataLocalProcessing( buffer, returned_buf_size,
									     last_chunk,
									     &keys, &values );
	}

	/*For first call need to create histogram and send to all map nodes
	 * To finally get dividers list which helps distribute of data to Reducers*/
	if ( userif->data.dividers_list == NULL ){
	    events.MapCreateHistogramSendEachToOtherCreateDividersList( chif, &userif->data, &keys );
	}

	/*based on dividers list which helps easy distribute data on the reduce nodes*/
	events.MapSendKeysValuesToAllReducers( chif, last_chunk, &keys, &values);

	FreeBufferData(&keys);
	FreeBufferData(&values);
    }while( last_chunk == 0 );

    WRITE_LOG("MapNodeMain Complete\n");

    return 0;
}


void MergeBuffersToNew( Buffer *newkeys, Buffer *newvalues,
			const Buffer *merge_buffers_keys, const Buffer *merge_buffers_values, int merge_count ){
    int merge_pos[merge_count];
    memset(merge_pos, '\0', sizeof(merge_pos) );
    int all_merged_condition = 0;
    int min_key_pos = 0;
    char minimal_value[newvalues->header.item_size];
    KeyType minimal_key = 0;
    KeyType temp_key = 0;
    const Buffer *current_buffer = NULL;
    assert( sizeof(minimal_key) == merge_buffers_keys->header.item_size ); //uint32_t key only allowed now
    do{
	all_merged_condition = 1; /*set flag always before next merge step*/
	minimal_key = 0;
	/*search minimal key for current keys buffers positions*/
	for( int i=0; i < merge_count; i++ ){
	    current_buffer = &merge_buffers_keys[i];
	    if ( merge_pos[i] < current_buffer->header.count ){
		GetBufferItem( current_buffer, merge_pos[i], &temp_key);
		if ( minimal_key == 0 ){
		    min_key_pos = 0;
		    minimal_key = temp_key;
		}
		if ( temp_key <= minimal_key ){
		    all_merged_condition = 0; /*not all data merged*/
		    min_key_pos = i;
		    minimal_key = temp_key;
		}
	    }
	}
	/*copy minimal key and linked value into merged keys,values buffers*/
	/*get value item and copy to merged values*/
	GetBufferItem( &merge_buffers_values[min_key_pos], merge_pos[min_key_pos], minimal_value);
	AddBufferItem( newkeys, &minimal_key );
	AddBufferItem( newvalues, minimal_value );
	merge_pos[min_key_pos]++;
    }while(all_merged_condition == 0);
}


int ReduceNodeMain(struct MapReduceUserIf *userif, struct ChannelsConfigInterface *chif ){
    WRITE_LOG("ReduceNodeMain\n");
    assert(userif);
    __userif = userif;

    /*alloc for merge*/
    int granularity = 1000;
    Buffer merged_keys;
    Buffer merged_values;
    memset( &merged_keys, '\0', sizeof(Buffer) );
    memset( &merged_values, '\0', sizeof(Buffer) );

    /*alloc for sorted*/
    Buffer all_keys;
    Buffer all_values;
    memset( &all_keys, '\0', sizeof(Buffer) );
    memset( &all_values, '\0', sizeof(Buffer) );

    /*get map_nodes_count*/
    int *map_nodes_list = NULL;
    int map_nodes_count = chif->GetNodesListByType( chif, EMapNode, &map_nodes_list);
    int excluded_map_nodes[map_nodes_count];
    memset( excluded_map_nodes, '\0', sizeof(excluded_map_nodes) );

    /*merge arrays of buffers*/
    int merge_count = map_nodes_count+1;
    int last_merge_item_index = map_nodes_count;
    /*allocation of recv_keys/values will complete at the data receive in next loop*/
    Buffer recv_keys[merge_count];
    Buffer recv_values[merge_count];
    memset( recv_keys, '\0', sizeof(recv_keys) );
    memset( recv_values, '\0', sizeof(recv_values) );

    /*read data from map nodes*/
    int bytes=0;
    int leave_map_nodes; /*is used as condition for do while loop*/
    do{
	leave_map_nodes = 0;
	for( int i=0; i < map_nodes_count; i++ ){
	    /*If current map node should send data*/
	    if ( excluded_map_nodes[i] != MAP_NODE_EXCLUDE ){
		struct UserChannel *channel = chif->Channel(chif, EMapNode, map_nodes_list[i], EChannelModeRead );
		assert(channel);
		WRITE_FMT_LOG( "fdr=%d\n", channel->fd );
		int fdr = channel->fd;
		/*read last data flag 0 | 1, if reducer receives 1 then it should
		 * exclude sender map node from communications*/
		bytes = read( fdr, &excluded_map_nodes[i], sizeof(excluded_map_nodes[i]) );
		assert( bytes == sizeof(excluded_map_nodes[i]) );
		WRITE_FMT_LOG( "read from map %d, map exclude flag=%d\n", map_nodes_list[i], (int)excluded_map_nodes[i] );
		/*set next wait loop condition*/
		if ( excluded_map_nodes[i] != MAP_NODE_EXCLUDE ){
		    leave_map_nodes = 1;
		}
		/*read keys header*/
		bytes = read( fdr, &recv_keys[i].header, sizeof(BufferHeader));
		assert( bytes == sizeof(BufferHeader) );
		/*alloc memory for keys data*/
		recv_keys[i].data = malloc( recv_keys[i].header.buf_size );
		WRITE_FMT_LOG( "read from map %d, keys header, buf_size=%d\n",
			       map_nodes_list[i], (int)recv_keys[i].header.buf_size );
		/*read keys data*/
		bytes = read( fdr, recv_keys[i].data, recv_keys[i].header.buf_size );
		assert( bytes == recv_keys[i].header.buf_size );
		/*read values header*/
		bytes = read( fdr, &recv_values[i].header, sizeof(BufferHeader));
		assert(bytes == sizeof(BufferHeader));
		/*alloc memory for values data*/
		recv_values[i].data = malloc( recv_values[i].header.buf_size );
		WRITE_FMT_LOG( "read from map %d, value header, buf_size=%d\n",
			       map_nodes_list[i], (int)recv_values[i].header.buf_size );
		/*read values data*/
		bytes = read( fdr, recv_values[i].data, recv_values[i].header.buf_size );
		assert(bytes == recv_values[i].header.buf_size );
		WRITE_FMT_LOG( "readed from map %d, data size =%d\n", map_nodes_list[i], bytes );
	    }
	    else{
		/*current map node excluded, free memory for unused buffers*/
		if ( recv_keys[i].data ){
		    free( recv_keys[i].data );
		    recv_keys[i].data = NULL;
		    recv_keys[i].header.count = 0;
		    recv_keys[i].header.buf_size = 0;
		}
		if ( recv_values[i].data ){
		    free( recv_values[i].data );
		    recv_values[i].data = NULL;
		    recv_values[i].header.count = 0;
		    recv_values[i].header.buf_size = 0;
		}
	    }
	}

	WRITE_LOG( "Prepare to merge" );

	/*portion of data received from map nodes; merge all received data stored in
	 * recv_keys, recv_values arrays in positions range "[0, map_nodes_cunt-1]";
	 * last_merge_item_index points to previous merge result stored in the same arrays */

	/*current result (all_keys,all_values) always copy to last array item before next merge
	 * ownership istransfered into new Buffers */
	recv_keys[last_merge_item_index] = all_keys;
	recv_values[last_merge_item_index] = all_values;

	/*Alloc buffers for merge results, and merge*/
	int result_granul = merged_keys.header.buf_size ? merged_keys.header.buf_size/merged_keys.header.item_size : granularity;
	assert( !AllocBuffer( &merged_keys, userif->data.keytype, result_granul ) );
	assert( !AllocBuffer( &merged_values, userif->data.valuetype, result_granul ) );
	MergeBuffersToNew( &merged_keys, &merged_values, recv_keys, recv_values, merge_count );
	WRITE_LOG( "merge complete" );

	/*Merge complete, so free merge input intermediate data*/
	if ( recv_keys[last_merge_item_index].data ){
	    free(recv_keys[last_merge_item_index].data);
	    recv_keys[last_merge_item_index].data = NULL;
	}
	if ( recv_values[last_merge_item_index].data ){
	    free(recv_values[last_merge_item_index].data);
	    recv_values[last_merge_item_index].data = NULL;
	}
	for ( int i=0; i < map_nodes_count; i++ ){
	    if ( recv_keys[i].data ){
		free( recv_keys[i].data );
		recv_keys[i].data = NULL;
		recv_keys[i].header.count = 0;
		recv_keys[i].header.buf_size = 0;
	    }
	    if ( recv_values[i].data ){
		free( recv_values[i].data );
		recv_values[i].data = NULL;
		recv_values[i].header.count = 0;
		recv_values[i].header.buf_size = 0;
	    }
	}
	/**********************/

	if ( userif->Combine ){
	    WRITE_FMT_LOG( "keys count before Combine: %d\n", (int)merged_keys.header.count );
	    userif->Combine( &merged_keys, &merged_values, &all_keys, &all_values );
	    FreeBufferData( &merged_keys );
	    FreeBufferData( &merged_values );
	    WRITE_FMT_LOG( "keys count after Combine: %d\n", (int)all_keys.header.count );
	}else{
	    int not_supported_yet = 0;
	    assert(not_supported_yet);
	}
    }while( leave_map_nodes != 0 );

    /*free intermediate keys,values buffers*/
    for(int i=0; i < merge_count; i++){
	if ( recv_keys[i].data )
	    free( recv_keys[i].data );
	if ( recv_values[i].data )
	    free( recv_values[i].data );
    }

    if( userif->Reduce ){
	/*user should output data into output file/s*/
	userif->Reduce( &all_keys, &all_values );
    }

    free(map_nodes_list);
    WRITE_LOG("ReduceNodeMain Complete\n");
    return 0;
}



