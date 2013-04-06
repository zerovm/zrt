/*
 * Implementation of Map, Reduce function for terrasort benchmark.
 * user_implem.c
 *
 *  Created on: 15.07.2012
 *      Author: yaroslav
 */

#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h> //size_t
#include <stdio.h> //printf
#include <assert.h>

#include "user_implem.h"
#include "mapreduce/map_reduce_lib.h"
#include "defines.h"
#include "elastic_mr_item.h"
#include "buffered_io.h"
#include "buffer.h"

static int 
ComparatorHash(const void *h1, const void *h2){
    return memcmp(h1,h2, sizeof(HASH_TYPE));
}

static int 
ComparatorElasticBufItemByHashQSort(const void *p1, const void *p2){
    return ComparatorHash( &((ElasticBufItemData*)p1)->key_hash,
			   &((ElasticBufItemData*)p2)->key_hash );
}

/*use ascii key as is for output*/
static char* 
PrintableHash( char* str, const uint8_t* hash, int size){
    memcpy(str, hash, sizeof(HASH_TYPE));
    return str;
}

static inline int 
CanReadRecord( const char* databegin, const char* dataend, int index ){
    if( &databegin[index] + TERRASORT_RECORD_SIZE <= dataend )
	return 1;
    else 
	return 0;
}



/*@return record size if OK or 0 if can't not read a whole record*/
static inline int 
DirectReadRecord( const char* databegin, const char* dataend, 
		  struct ElasticBufItemData* record, int *index ){
    const char* c = &databegin[*index];

    /*set key data: pointer + key data length. using pointer to existing data*/
    record->key_data.addr = (uintptr_t)c;
    record->key_data.size = HASH_SIZE;
    record->own_key = EDataNotOwned;
    /*save as hash record key without changes to variable length struct member*/
    memcpy( &record->key_hash, c, HASH_SIZE );
    /*interpret elasticdata->value.addr as 4bytes pointer and save data as binary*/
    record->value.size = TERRASORT_RECORD_SIZE - HASH_SIZE;
    record->value.addr = (uintptr_t)c + HASH_SIZE;
    record->own_value = EDataNotOwned;
    /*update current pos*/
    (*index) += TERRASORT_RECORD_SIZE;
    return TERRASORT_RECORD_SIZE;
}

/*******************************************************************************
 * User map for MAP REDUCE
 * Readed data will organized as MrItem with a 
 * 10bytes key, 90bytes data and 10bytes hash exactly equal to the key
 * @param size size of data must be multiple on 100, 
 * set MAP_CHUNK_SIZE env variable properly*/
int Map(const char *data, 
	size_t size, 
	int last_chunk, 
	Buffer *map_buffer ){
    ElasticBufItemData* elasticdata;
    int current_pos = 0;
    while( CanReadRecord( data, data+size, current_pos) == 1 ){
	/*it's guarantied that item space will reserved in buffer, and no excessive
	  copy doing, elasticdata points directly to buffer item*/
	elasticdata = (ElasticBufItemData*)
	    BufferItemPointer(map_buffer, 
			      AddBufferItemVirtually(map_buffer) );
	/*read directly into cell of array*/
	DirectReadRecord( data, data+size, elasticdata, &current_pos );
    }
    /*return real handled data pos*/
    return current_pos;
}

int Reduce( const Buffer *reduced_buffer ){
    BufferedIOWrite* bio = AllocBufferedIOWrite( malloc(IO_BUF_SIZE), IO_BUF_SIZE);

    /*declare buf item to use it as current loop item*/
    ElasticBufItemData* elasticdata;
    HASH_TYPE prev_key;
    for ( int i=0; i < reduced_buffer->header.count; i++ ){
	elasticdata = (ElasticBufItemData*)BufferItemPointer( reduced_buffer, i );

	bio->write(bio, STDOUT, (void*)elasticdata->key_data.addr, elasticdata->key_data.size);
	bio->write(bio, STDOUT, (void*)elasticdata->value.addr, elasticdata->value.size);

	/*test data sorted by hash*/
	HASH_TYPE* keyhash = (HASH_TYPE*)&elasticdata->key_hash;
	if ( i>0 && ComparatorHash(&prev_key, keyhash) >0 ){
	    bio->flush_write(bio, STDOUT);
	    printf("test failed prev_key=%s, key=%s\n", 
		   PrintableHash(alloca(HASH_STR_LEN), (const uint8_t *)&prev_key, HASH_SIZE),
		   PrintableHash(alloca(HASH_STR_LEN), (const uint8_t *)keyhash, HASH_SIZE) );
	    fflush(0);
	    exit(-1);
	}

	memcpy( &prev_key, &elasticdata->key_hash, HASH_SIZE );
	TRY_FREE_MRITEM_DATA(elasticdata);  
    }
    bio->flush_write(bio, STDOUT);

    free(bio->data.buf);     /*free buffer in this way because not saved pointer*/
    WRITE_LOG("OK");
    free(bio);
    return 0;
}


void InitInterface( struct MapReduceUserIf* mr_if ){
    memset( mr_if, '\0', sizeof(struct MapReduceUserIf) );
    PREPARE_MAPREDUCE(mr_if, 
		      Map, 
		      NULL, 
		      Reduce, 
		      ComparatorElasticBufItemByHashQSort,
		      ComparatorHash,
		      PrintableHash,
		      0,
		      ALIGNED_RECORD_SIZE,
		      HASH_SIZE );
}


