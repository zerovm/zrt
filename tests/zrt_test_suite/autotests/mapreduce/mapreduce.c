/*mapreduce library test*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <error.h>
#include <errno.h>
#include <assert.h>
#include <alloca.h>

#include "../channels/test_channels.h"
#include "map_reduce_lib.h"
#include "elastic_mr_item.h"
#include "buffer.h"


#define HISTOGRAM_STEP 10
#define GRANULARITY 10
#define ITEM_SIZE sizeof(				\
			 struct{			\
			     BinaryData     key_data;	\
			     BinaryData     value;	\
			     uint8_t        own_key;	\
			     uint8_t        own_value;  \
			     uint32_t       key_hash;	\
			 })
#define VALUE_TYPE uint32_t
#define HASH_TYPE  unsigned short int

#define SET_BINARY_STRING(binary,str){					\
	(binary).addr = (uintptr_t) (str);				\
	(binary).size = strlen((const char*)item->key_data.addr);	\
    }

#define SET_BINARY_DATA(binary,data_p,binarysize){	\
	(binary).addr = (uintptr_t) (data_p);		\
	(binary).size = (binarysize);			\
    }

#define SET_KEY_HASH(key_hash_p, hash, hash_type){		\
	hash_type temphash = hash;				\
	memcpy( (key_hash_p), &temphash, sizeof(hash_type) );	\
    }

static char* 
PrintableHash( char* str, const uint8_t* hash, int size){
    snprintf(str, size*2+1, "%X", *(uint32_t*)hash);
    return str;
}

static int 
HashComparator(const void *h1, const void *h2){
    if      ( *(HASH_TYPE*)h1 < *(HASH_TYPE*)h2 ) return -1;
    else if ( *(HASH_TYPE*)h1 > *(HASH_TYPE*)h2 ) return 1;
    else return 0;
}

static int
AddItemsStrKey(Buffer* array, const char* nullstrkey, VALUE_TYPE value, HASH_TYPE hash,
	      int duplicate_items_count){
    struct ElasticBufItemData* item = alloca(array->header.item_size);
    for(int i=0; i < duplicate_items_count; i++){
	SET_BINARY_STRING( item->key_data, nullstrkey);
	SET_BINARY_DATA( item->value, &value, sizeof(VALUE_TYPE) );
	SET_KEY_HASH( &item->key_hash, hash, HASH_TYPE);
	AddBufferItem( array, item);
    }
    return duplicate_items_count;
}

static int
AddItemsIntoBuffer(Buffer* array){
    int ret=0;
    ret+=AddItemsStrKey( array, "hello", 0xf00001, 1, 5 );
    ret+=AddItemsStrKey( array, "pello", 0x0f0001, 2, 21 );
    ret+=AddItemsStrKey( array, "bello", 0x00f001, 3, 56 );
    ret+=AddItemsStrKey( array, "good",  0x000f02, 4, 56 );
    return ret;
}

static Buffer*
TestHistogramGetDividers(struct MapReduceUserIf* mif, int count){
    int ret;
    Histogram *histograms = alloca(sizeof(Histogram)*count);
    Buffer* array_of_buffers = alloca(sizeof(Buffer)*count);
    Buffer* divider_hashes = malloc(sizeof(Buffer));
    AllocBuffer( divider_hashes, sizeof(HASH_TYPE), 
		 count /*exact count of dividers we want to produce*/ );
    Buffer* current_buffer;
    int all_buffers_items_count=0;
    for(int i=0; i < count; i++){
	ret = AllocBuffer( &histograms[i].buffer, sizeof(HASH_TYPE), GRANULARITY );
	assert(!ret);

	current_buffer = &array_of_buffers[i];
	ret = AllocBuffer( current_buffer, ITEM_SIZE, GRANULARITY );
	assert(!ret);

	int buf_items_count = AddItemsIntoBuffer(current_buffer);
	assert(buf_items_count== current_buffer->header.count);
	all_buffers_items_count += buf_items_count;

	int expected_items_in_histogram = buf_items_count/HISTOGRAM_STEP;
	if ( buf_items_count > expected_items_in_histogram*HISTOGRAM_STEP )
	    ++expected_items_in_histogram;

	int histogram_items_count = GetHistogram( mif,
						  current_buffer,
						  HISTOGRAM_STEP,
						  &histograms[i] );
	/*GetHistogram result testing*/
	TEST_OPERATION_RESULT( histogram_items_count==expected_items_in_histogram,
			       &ret, ret==1);
    }

    GetReducersDividerArrayBasedOnSummarizedHistograms( mif,
							histograms,
    							count,
    							divider_hashes );
    for(int i=0; i < count; i++){
    	FreeBufferData(&array_of_buffers[i]);
    	FreeBufferData(&histograms[i].buffer);
    }
    return divider_hashes;
}

int main(int argc, char** argv){
    int ret;
    struct MapReduceUserIf mif;
    PREPARE_MAPREDUCE( &mif, 
		       NULL, /*map*/
		       NULL, /*combine*/
		       NULL, /*reduce*/
		       HashComparator,
		       PrintableHash,
		       0,    /*value addr is not data, use as pointer*/
		       ITEM_SIZE, 
		       sizeof(HASH_TYPE) );
    Buffer* dividers = NULL;
    HASH_TYPE maxhash=~0; //0xffff

    /*Test histogram & dividers creation for 1 mapper & 1 reducer*/
    dividers = TestHistogramGetDividers(&mif, 1);
    fprintf(stderr, "dividers count=%d\n", dividers->header.count);
    TEST_OPERATION_RESULT(
    			  (*(HASH_TYPE*)BufferItemPointer(dividers, dividers->header.count-1))
    			  == maxhash, &ret, ret==1);
    FreeBufferData(dividers);
    free(dividers);

    /*Pending tests*/
    /*Test histogram & dividers creation for 2 mappers & 2 reducers*/
    //TestHistogramGetDividers(&mif, 2);
    /*Test histogram & dividers creation for 3 mappers & 3 reducers*/
    //TestHistogramGetDividers(&mif, 3);
    return 0;
}
