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
#include "mr_defines.h"
#include "buffer.h"


#define HISTOGRAM_STEP 10
#define GRANULARITY 10
#define ITEM_SIZE sizeof(				\
			 struct{			\
			     BinaryData     key_data;	\
			     BinaryData     value;	\
			     uint8_t        own_key;	\
			     uint8_t        own_value;  \
			     HASH_TYPE      key_hash;	\
			 })
#define VALUE_TYPE uint32_t
#define HASH_TYPE  uint16_t

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
static void
AddItemStrKey(Buffer* array, const char* nullstrkey, VALUE_TYPE value, HASH_TYPE hash){
    struct ElasticBufItemData* item = alloca(array->header.item_size);
    SET_BINARY_STRING( item->key_data, nullstrkey);
    SET_BINARY_DATA( item->value, &value, sizeof(VALUE_TYPE) );
    SET_KEY_HASH( &item->key_hash, hash, HASH_TYPE);
    AddBufferItem( array, item);
}


/*Mapreduce user interface functions*/

static char* 
PrintableHash( char* str, const uint8_t* hash, int size){
    sprintf(str, "%X", *(HASH_TYPE*)hash);
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
#define TEST_CASE_2 "2"
#define TEST_CASE_3 "3"
#define TEST_CASE_4 "4"
static int 
Map(const char *data, size_t size, int last_chunk, Buffer *map_buffer ){
    int i;
    char temp[100];
    int unhandled_data_pos=0;
    /*instead of parsing we will generate only some data depending on
      first character in string. Be carefull if changing code below, 
      tests results can be vary on it*/
    switch(*data){
    case '1':
	/*add items to map*/
	for(i=0; i < 50; i++){
	    snprintf(temp, sizeof(temp), "%s-%d", "hello", i);
	    AddItemStrKey( map_buffer, temp, 0xf00000+i, i*2 /*hash*/ );
	}
	unhandled_data_pos=size; /*all data processed*/
	break;
    case '2':
	/*add items to map*/
	for(i=0; i < 50; i++){
	    snprintf(temp, sizeof(temp), "%s-%d", "hello", i);
	    AddItemStrKey( map_buffer, temp, 0xf00000+i, i*2 /*hash*/ );
	}
	for(i=0; i < 5000; i++){
	    snprintf(temp, sizeof(temp), "%s-%d", "bello", i);
	    AddItemStrKey( map_buffer, temp, 0xf00000+i, i*2 /*hash*/ );
	}
	unhandled_data_pos=size; /*all data processed*/
	break;
    case '3':
	AddItemStrKey( map_buffer, temp, 0xf00000, 0xE /*hash*/ );
	AddItemStrKey( map_buffer, temp, 0xf00000, 0x1 /*hash*/ );
	break;
    case '4':
	AddItemStrKey( map_buffer, temp, 0xf00000, 0xE /*hash*/ );
	AddItemStrKey( map_buffer, temp, 0xf00000, 0x10 /*hash*/ );
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
TestHistogramGetDividers(struct MapReduceUserIf* mif, int count, const char* testcase){
    int ret;
    HASH_TYPE maxhash=~0; //0xffff
    Histogram *histograms = alloca(sizeof(Histogram)*count);
    mif->data.histograms_list = histograms;
    mif->data.histograms_count = count;

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

	MapInputDataLocalProcessing( mif, testcase, strlen(testcase), 1, current );
	all_buffers_items_count += current->header.count;

	int expected_items_in_histogram = current->header.count/HISTOGRAM_STEP;
	if ( current->header.count > expected_items_in_histogram*HISTOGRAM_STEP )
	    ++expected_items_in_histogram;

	int histogram_items_count = GetHistogram( mif,
						  current,
						  HISTOGRAM_STEP,
						  &histograms[i] );
	fprintf(stderr,"histogram_items_count=%d, expected_items_in_histogram=%d\n",
		      histogram_items_count, expected_items_in_histogram);
	/*GetHistogram result testing*/
	TEST_OPERATION_RESULT( histogram_items_count==expected_items_in_histogram,
			       &ret, ret==1);
	/*Test histogram sorted or not*/
	HASH_TYPE minitem=0;
	for(int j=0; j < histograms[i].buffer.header.count; j++ ){
	    HASH_TYPE currentitem = *(HASH_TYPE*)BufferItemPointer(&histograms[i].buffer, j);
	    assert( minitem < currentitem );
	    minitem = currentitem;
	}
    }

    GetReducersDividerArrayBasedOnSummarizedHistograms( mif,
							histograms,
    							count,
    							divider_hashes );
    fprintf(stderr, "dividers count=%d\n", divider_hashes->header.count);

    /*test dividers items count*/
    TEST_OPERATION_RESULT(count==divider_hashes->header.count, 
			  &ret, ret==1);

    /*test divider items, except last. skip test for array size=1 because single item
     we asssume as last*/
    if ( count > 1 ){
	int minindexes[count];
	memset(minindexes, '\0', sizeof(minindexes));
	size_t divider_block_items_count 
	    = all_buffers_items_count / count;
	/*create &fill control dividers array o use it as etalon when compare with
	 array returned by mapreducelib*/
	Buffer control_dividers;
	AllocBuffer( &control_dividers, sizeof(HASH_TYPE), 1 );
	int itemcount=0;
	HASH_TYPE currenthash;
	do {
	    HASH_TYPE minhash = ~0;
	    /*count by data arrays*/
	    for ( int i=0; i < count; i++ ){
		/*locate minimum hash item, using as current items with index 
		  in minindexes array*/
		GetBufferItem( &histograms[i].buffer, minindexes[i], &currenthash);
		if ( currenthash <= minhash ){
		    minhash=currenthash;
		    itemcount += histograms[i].step_hist_common;
		    minindexes[i]++;
		    if ( itemcount % divider_block_items_count == 0 ){
			fprintf(stderr, "control_divider #%d=%x\n", 
				control_dividers.header.count, minhash );
			AddBufferItem(&control_dividers, &minhash);
		    }
		}
	    }
	}while(itemcount < all_buffers_items_count);

	/*test divider items*/
	int test_count=control_dividers.header.count;
	if ( all_buffers_items_count%divider_block_items_count == 0 &&
	     all_buffers_items_count){
	    /*if items count is aligned just ignore last item, it anyway will be tested
	     later*/
	    --test_count;
	}

	HASH_TYPE item1, item2;
	for( int i=0; i < test_count; i++ ){
	    GetBufferItem( divider_hashes, i, &item1 );
	    GetBufferItem( &control_dividers, i, &item2 );
	    fprintf(stderr, "item1=%x, item2=%x\n", item1, item2);
	    TEST_OPERATION_RESULT( item1==item2, &ret, ret==1);
	}
    	FreeBufferData(&control_dividers);
    }

    /*test last divider item, it should has maximum possible value*/
    TEST_OPERATION_RESULT( (*(HASH_TYPE*)BufferItemPointer(divider_hashes, 
							   divider_hashes->header.count-1))
    			  == maxhash, &ret, ret==1);

    for(int i=0; i < count; i++){
    	FreeBufferData(&array_of_buffers[i]);
    	FreeBufferData(&histograms[i].buffer);
    }
    return divider_hashes;
}

Buffer*
TestMerge(struct MapReduceUserIf *mif){
    int ret;
    Buffer* current;
    int count = 2; /*only two arrays will be merged*/
    Buffer* array_of_buffers = alloca(sizeof(Buffer)*count);
    Buffer* dest = malloc(sizeof(Buffer));

    //array1
    current = &array_of_buffers[0];
    ret = AllocBuffer( current, ITEM_SIZE, GRANULARITY );
    assert(!ret);
    MapInputDataLocalProcessing( mif, TEST_CASE_3, strlen(TEST_CASE_3), 1, current );

    //array2
    current = &array_of_buffers[1];
    ret = AllocBuffer( current, ITEM_SIZE, GRANULARITY );
    assert(!ret);
    MapInputDataLocalProcessing( mif, TEST_CASE_4, strlen(TEST_CASE_4), 1, current );

    ////////////
    ret = AllocBuffer( dest, ITEM_SIZE, GRANULARITY );
    MergeBuffersToNew( mif, dest, array_of_buffers, count);

    /*Test items sorted or not by hash*/
    HASH_TYPE minitem=0;
    ElasticBufItemData* currentitem;
    for(int j=0; j < dest->header.count; j++ ){
	currentitem = (ElasticBufItemData*)BufferItemPointer(dest, j);
	fprintf(stderr, "prev=%X, current=%X\n", minitem, *(HASH_TYPE*)&currentitem->key_hash );
	assert( HASH_CMP(mif, &minitem, &currentitem->key_hash ) <= 0 );
	HASH_COPY( &minitem, &currentitem->key_hash, sizeof(HASH_TYPE));
    }
    
    return dest;
}

struct MapReduceUserIf* AllocMapReduce()
{
    struct MapReduceUserIf* mif = malloc(sizeof(struct MapReduceUserIf));
    PREPARE_MAPREDUCE( mif, 
		       Map, /*map*/
		       Combine, /*combine*/
		       NULL, /*reduce*/
		       ComparatorElasticBufItemByHashQSort,
		       ComparatorHash,
		       PrintableHash,
		       1,    /*value addr is data*/
		       ITEM_SIZE, 
		       sizeof(HASH_TYPE) );
    return mif;
}

int main(int argc, char** argv){
    int ret;
    struct MapReduceUserIf *mif;
    Buffer* dividers = NULL;

    /*Test histogram & dividers creation for 1 mapper & 1 reducer*/
    mif = AllocMapReduce();
    dividers = TestHistogramGetDividers(mif, 1, TEST_CASE_1);
    FreeBufferData(dividers);
    free(dividers);
    free(mif);

    /*Pending tests*/
    /*Test histogram & dividers creation for 2 mappers & 2 reducers*/
    mif = AllocMapReduce();
    dividers = TestHistogramGetDividers(mif, 2, TEST_CASE_1);
    FreeBufferData(dividers);
    free(dividers);
    free(mif);

    /*Test histogram & dividers creation for 3 mappers & 3 reducers*/
    mif = AllocMapReduce();
    dividers = TestHistogramGetDividers(mif, 3, TEST_CASE_2);
    FreeBufferData(dividers);
    free(dividers);
    free(mif);

    /*Merge buffers test. This is test for bug in merge of items which have 16bit hash */
    mif = AllocMapReduce();
    Buffer* merged = TestMerge(mif);
    FreeBufferData(merged);
    free(merged);
    free(mif);

    (void)ret;
    return 0;
}
