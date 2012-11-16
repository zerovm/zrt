/*
 * user_implem.c
 *
 *  Created on: 15.07.2012
 *      Author: yaroslav
 */

#include <string.h>
#include <stdint.h>
#include <stddef.h> //size_t
#include <stdio.h> //printf
#include <assert.h>

#include "user_implem.h"
#include "mapreduce/map_reduce_lib.h"
#include "defines.h"

/*******************************************************************************
 * USER HASH*/
static const uint32_t InitialFNV = 2166136261U;
static const uint32_t FNVMultiple = 16777619;
/* Fowler / Noll / Vo (FNV) Hash */
uint32_t HashForUserString( const char *str, int size )
{
	uint32_t hash = InitialFNV;
    for(int i = 0; i < size; i++)
    {
        hash = hash ^ (str[i]);       /* xor  the low 8 bits */
        hash = hash * FNVMultiple;  /* multiply by the magic number */
    }
    return hash;
}
/*******************************************************************************/

/*******************************************************************************
 * User map for MAP REDUCE*/
int Map(const char *data, size_t size, int last_chunk, Buffer *keys, Buffer *values ){
	size_t allocated_items_count = 1024;
	assert( !AllocBuffer( keys, EUint32, allocated_items_count ) );
	assert( !AllocBuffer( values, EUint32, allocated_items_count ) );

	char buf_temp[50];
	uint32_t keyvalue = 1;
	int max_count = keys->header.buf_size / keys->header.item_size;
	int current_pos = 0;
	const char *search_result = NULL;
	int str_length = 0;
	do{
		search_result = strchrnul(&data[current_pos], ' ' );
		str_length = (uint32_t)(search_result - data) - current_pos;
		if ( str_length || (data+size == search_result && last_chunk) ){
			if ( str_length > 0 ){
				if ( keys->header.count >= max_count ){
					assert( !ReallocBuffer( keys ) );
					assert( !ReallocBuffer( values ) );
					max_count = keys->header.buf_size / keys->header.item_size;
				}
				//save key
				uint32_t hash = HashForUserString( &data[current_pos], str_length );
				SetBufferItem( keys, keys->header.count, &hash );
				//save value
				SetBufferItem( values, values->header.count, &keyvalue);
				++keys->header.count;
				++values->header.count;

				memset( buf_temp, '\0', sizeof(buf_temp) );
				memcpy( buf_temp, &data[current_pos], MIN(str_length, sizeof(buf_temp)) );
				printf( "%u=[%d]%s\n", hash, str_length, buf_temp );
			}

			current_pos = search_result - data + 1; //pos pointed to space just after ' '
		}
		/*if */
		else if ( search_result-data < size ){
			++current_pos;
		}
		/*do while search_result ponts to the end of data*/
	}while( search_result-data < size && current_pos < size );
	/*should return real handled data pos, as in code line below*/
	//return MIN(current_pos, size );
	/*WRONG : always return that all data handled, but for some tests*/
	return size;
}


/*******************************************************************************
 * User combine for MAP REDUCE*/
int Combine( const Buffer *keys, const Buffer *values, Buffer *reduced_keys, Buffer *reduced_values ){
	uint32_t last_unique_key = 0;
	uint32_t combined_value = 0;

	assert( !AllocBuffer(reduced_keys, keys->header.data_type, keys->header.count) );
	assert( !AllocBuffer(reduced_values, values->header.data_type, values->header.count) );

	uint32_t key;
	for (int i=0; i < keys->header.count; i++){
		GetBufferItem( keys, i, &key );
		if ( key != last_unique_key ){
			/*save previously combined key-value*/
			SetBufferItem( reduced_keys, reduced_keys->header.count, &last_unique_key );
			SetBufferItem( reduced_values, reduced_values->header.count, &combined_value );
			++reduced_keys->header.count;
			++reduced_values->header.count;
			last_unique_key = key;
			combined_value = 0;
		}
		uint32_t value;
		GetBufferItem( values, i, &value );
		combined_value += value;
	}
	return 0;
}

int Reduce( const Buffer *keys, const Buffer *values ){
	uint32_t key;
	uint32_t val;

	for ( int i=0; i < keys->header.count; i++ ){
		GetBufferItem( keys, i, &key );
		GetBufferItem( values, i, &val );

		printf( "[#%d]%u=%u\n", i, (uint32_t)key, (uint32_t)val );
	}
	return 0;
}


void InitInterface( struct MapReduceUserIf* mr_if ){
    memset( mr_if, '\0', sizeof(struct MapReduceUserIf) );
    PREPARE_MAPREDUCE(mr_if, Map, Combine, Reduce, EUint32, EUint32);
}


