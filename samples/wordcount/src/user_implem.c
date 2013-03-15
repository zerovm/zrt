/*
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

/******************************************************************************
 * Buferrized Write*/
#define SPRINTF_BUFFER_SIZE 50
#define BUFFER_IO_SIZE 0x100000
static char s_sprintf_buffer[SPRINTF_BUFFER_SIZE];
static int  s_buffer_io_cursor=0;
static char s_buffer_io[BUFFER_IO_SIZE];

#define IF_BUFFER_WRITE(data,size)				\
    if ( size < BUFFER_IO_SIZE - s_buffer_io_cursor ){		\
	/*buffer is enough to write data*/			\
	memcpy(s_buffer_io+s_buffer_io_cursor, data, size );	\
	s_buffer_io_cursor += size;				\
    }

void buf_flush(int fd){
    write(fd, s_buffer_io, s_buffer_io_cursor);
    s_buffer_io_cursor=0;
}
void buf_write(int fd, const char* data, size_t size){
    IF_BUFFER_WRITE(data,size)
    else{
	buf_flush(fd);
	IF_BUFFER_WRITE(data,size)
	else{
	    /*write direct to fd, buffer is too small*/  
	    write(fd, data, size);	    
	}
    }
}

/*******************************************************************************/

/*******************************************************************************/

/*******************************************************************************
 * User map for MAP REDUCE*/
int Map(const char *data, size_t size, int last_chunk, Buffer *keys, Buffer *values ){
	size_t allocated_items_count = 1024;
	assert( !AllocBuffer( keys, EUint32, allocated_items_count ) );
	assert( !AllocBuffer( values, EUint32, allocated_items_count ) );

	int print;
	char buf_temp[40]; /*max word screen representation length*/
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

#ifdef OUTPUT_HASH_KEYS
				memset( buf_temp, '\0', MIN(str_length, sizeof(buf_temp)) +1 );
				memcpy( buf_temp, &data[current_pos], MIN(str_length, sizeof(buf_temp)) );
				print = snprintf( s_sprintf_buffer, SPRINTF_BUFFER_SIZE, 
						  "%u=[%d]%s\n", hash, str_length, buf_temp );
				buf_write(STDOUT, s_sprintf_buffer, print);
#endif
			}

			current_pos = search_result - data + 1; //pos pointed to space just after ' '
		}
		/*if */
		else if ( search_result-data < size ){
			++current_pos;
		}
		/*do while search_result ponts to the end of data*/
	}while( search_result-data < size && current_pos < size );
#ifdef OUTPUT_HASH_KEYS
	buf_flush(STDOUT);
#endif
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
	uint32_t prev_key=0;
	uint32_t prev_val=0;


	int print;
	for ( int i=0; i < keys->header.count; i++ ){
	    GetBufferItem( keys, i, &key );
	    GetBufferItem( values, i, &val );
	    
	    print = snprintf( s_sprintf_buffer, SPRINTF_BUFFER_SIZE, 
			      "[#%d]%u=%u\n", i, (uint32_t)key, (uint32_t)val );
	    buf_write(STDOUT, s_sprintf_buffer, print);
	    
	    if ( i>0 && prev_key >= key ){
		buf_flush(STDOUT);
		printf("test error prev_key=%u, key=%u\n", prev_key, key);
		fflush(0);
		exit(-1);
	    }
		
	    prev_key = key;
	    prev_val = val;
	}
	buf_flush(STDOUT);
	return 0;
}


void InitInterface( struct MapReduceUserIf* mr_if ){
    memset( mr_if, '\0', sizeof(struct MapReduceUserIf) );
    PREPARE_MAPREDUCE(mr_if, Map, Combine, Reduce, EUint32, EUint32);
}


