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


/**helper functions*/
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


/*Mapreduce user interface functions*/

static char* 
PrintableHash( char* str, const uint8_t* hash, int size){
    snprintf(str, size*2+1, "%X", *(uint32_t*)hash);
    return str;
}

static int 
ComparatorHash(const void *h1, const void *h2){
    if      ( *(HASH_TYPE*)h1 < *(HASH_TYPE*)h2 ) return -1;
    else if ( *(HASH_TYPE*)h1 > *(HASH_TYPE*)h2 ) return 1;
    else return 0;
}

static int 
ComparatorElasticBufItemByHashQSort(const void *p1, const void *p2){
    return ComparatorHash( &((ElasticBufItemData*)p1)->key_hash,
			   &((ElasticBufItemData*)p2)->key_hash );
}

#define TEST_CASE_1 "1"
static int 
Map(const char *data, size_t size, int last_chunk, Buffer *map_buffer ){
    int unhandled_data_pos=0;
    /*instead of parsing we will generate only some data depending on
      first character in string */
    switch(*data){
    case '1':
	/*do not change list below, tests are based on it*/
	AddItemsStrKey( map_buffer, "hello", 0xf00001, 1, 5 );
	AddItemsStrKey( map_buffer, "pello", 0x0f0001, 2, 21 );
	AddItemsStrKey( map_buffer, "bello", 0x00f001, 3, 56 );
	AddItemsStrKey( map_buffer, "good",  0x000f02, 4, 56 );
	unhandled_data_pos=size; /*all data processed*/
	break;
    default:
	break;
    }
    return unhandled_data_pos;
}

int Combine( const Buffer *map_buffer, Buffer *reduce_buffer ){
    int combined_count=0;

    ElasticBufItemData* current;
    ElasticBufItemData* combine = alloca(map_buffer->header.item_size);
    if ( map_buffer->header.count ){
	GetBufferItem( map_buffer, 0, combine );
    }

    /*go through items of sorted map_buffer*/
    for (int i=0; i < map_buffer->header.count; i++){
	/*current loop item*/
	current = (ElasticBufItemData*) BufferItemPointer( map_buffer, i );
	/*if current item can be added into reduce_buffer*/
	if ( *(HASH_TYPE*)&current->key_hash != *(HASH_TYPE*)&combine->key_hash ){
 	    /*save previously combined item*/
	    AddBufferItem( reduce_buffer, combine );
	    GetBufferItem( map_buffer, i, combine );
	    combined_count=1;
	}
	/*if previous item and new one has the same key then we need 
	 *to modify value of item to reduce it*/
	else{
	    ++combined_count;
	    /*update value only for items that now combining more than one item*/
	    if ( combined_count > 1 ){
		combine->value.addr += current->value.addr;
	    }
	}
    }

    /*save last combined value if combined item not yet added*/
    if (combined_count>0){
	AddBufferItem( reduce_buffer, combine );
    }
    return 0;
}

/*functions related to histogram testing */

static Buffer*
TestHistogramGetDividers(struct MapReduceUserIf* mif, int count){
    int ret;
    Histogram *histograms = alloca(sizeof(Histogram)*count);
    Buffer* array_of_buffers = alloca(sizeof(Buffer)*count);
    Buffer* divider_hashes = malloc(sizeof(Buffer));
    AllocBuffer( divider_hashes, sizeof(HASH_TYPE), 
		 count /*exact count of dividers we want to produce*/ );
    Buffer* current;
    int all_buffers_items_count=0;

    for(int i=0; i < count; i++){
	ret = AllocBuffer( &histograms[i].buffer, sizeof(HASH_TYPE), GRANULARITY );
	assert(!ret);

	current = &array_of_buffers[i];
	ret = AllocBuffer( current, ITEM_SIZE, GRANULARITY );
	assert(!ret);

	MapInputDataLocalProcessing( mif, TEST_CASE_1, strlen(TEST_CASE_1), 1, current );
	all_buffers_items_count += current->header.count;

	int expected_items_in_histogram = all_buffers_items_count/HISTOGRAM_STEP;
	if ( all_buffers_items_count > expected_items_in_histogram*HISTOGRAM_STEP )
	    ++expected_items_in_histogram;

	int histogram_items_count = GetHistogram( mif,
						  current,
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
		       Map, /*map*/
		       Combine, /*combine*/
		       NULL, /*reduce*/
		       ComparatorElasticBufItemByHashQSort,
		       ComparatorHash,
		       PrintableHash,
		       1,    /*value addr is data*/
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
    /* TestHistogramGetDividers(&mif, 2); */
    /*Test histogram & dividers creation for 3 mappers & 3 reducers*/
    /* TestHistogramGetDividers(&mif, 3); */

    return 0;
}
