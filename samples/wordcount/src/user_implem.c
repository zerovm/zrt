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
#include "elastic_mr_item.h"
#include "buffered_io.h"
#include "buffer.h"

/*******************************************************************************
 * USER HASH*/
#ifdef HASH_TYPE_UINT16
static const HASH_TYPE InitialFNV = 21661U;
static const HASH_TYPE FNVMultiple = 16719;
#else
static const HASH_TYPE InitialFNV = 2166136261U;
static const HASH_TYPE FNVMultiple = 16777619;
#endif
/* Fowler / Noll / Vo (FNV) Hash */
HASH_TYPE HashForUserString( const char *str, int size )
{
    HASH_TYPE hash = InitialFNV;
    for(int i = 0; i < size; i++)
	{
	    hash = hash ^ (str[i]);       /* xor  the low 8 bits */
	    hash = hash * FNVMultiple;  /* multiply by the magic number */
	}
    return hash;
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

static char* 
PrintableHash( char* str, const uint8_t* hash, int size){
#ifdef HASH_TYPE_UINT64
    snprintf(str, HASH_STR_LEN, "%X%X", 
	     (uint32_t)(*(HASH_TYPE*)hash>>32), (uint32_t)(*(HASH_TYPE*)hash));
#else
    snprintf(str, HASH_STR_LEN, "%X", *(HASH_TYPE*)hash);
#endif
    return str;
}


/*return len of the return string via word pointer*/
static inline int 
GetWordStr( const char* databegin, const char* dataend, const char** word, 
	    int *index ){
    *word = NULL;
    int wordlen=0;
    const char* c = &databegin[*index];
    while( c < dataend && !wordlen ){
	if ( (*c >= 'A' && *c <= 'Z') || (*c >= 'a' && *c <= 'z') || (*c== '-') ){
	    if ( *word == NULL )
		*word = c;
	}
	else if (*word != NULL)
	    wordlen = (int)(c-*word);
	c++;
    }
    *index = c - databegin;
    return wordlen;
}

/*******************************************************************************
 * User map for MAP REDUCE*/
int Map(const char *data, 
	size_t size, 
	int last_chunk, 
	Buffer *map_buffer ){

#ifdef OUTPUT_HASH_KEYS
    char printf_buffer[0x100];
    BufferedIOWrite* bio = AllocBufferedIOWrite( malloc(IO_BUF_SIZE), IO_BUF_SIZE);
    char* hashstr = alloca(HASH_STR_LEN);
    int print;
#endif

    ElasticBufItemData* elasticdata;
    int current_pos = 0;
    const char* word;
    int wordlen = 0;
    while( (wordlen=GetWordStr( data, data+size, &word, &current_pos ) ) || 
	   (!wordlen && word && last_chunk) ){
	
	if (!wordlen && word && last_chunk){
	    /*for last chunk calculate wordlen*/
	    wordlen = size - (int)(word-data);
	}

	/*it's guarantied that item space will reserved in buffer, and no excessive
	  copy doing, elasticdata points directly to buffer item*/
	elasticdata = (ElasticBufItemData*)
	    BufferItemPointer(map_buffer, 
			      AddBufferItemVirtually(map_buffer) );

	/*set key data: pointer + key data length. using pointer to existing data*/
	elasticdata->key_data.addr = (uintptr_t)word;
	elasticdata->key_data.size = wordlen;
	elasticdata->own_key = EDataNotOwned;
	//	WRITE_FMT_LOG("\\[%d]%d\\\n", current_pos, wordlen);
	
	/*save hash key to variable length struct member*/
	HASH_TYPE temphash = HashForUserString( word, wordlen );
	memcpy( &elasticdata->key_hash, &temphash, HASH_SIZE );
	
	/*save value depends on wordcount config*/
	if ( VALUE_ADDR_AS_DATA == 1){
	    /*use elasticdata->value.addr as 4byte value*/
	    elasticdata->value.addr = DEFAULT_INT_VALUE;
	    elasticdata->own_value = EDataNotOwned;
	}
	else{
	    /*interpret elasticdata->data as 4bytes pointer and save data as binary*/
	    elasticdata->value.size = strlen(DEFAULT_STR_VALUE);
	    elasticdata->value.addr = (uintptr_t)malloc( elasticdata->value.size );
	    elasticdata->own_value = EDataOwned;
	    memcpy( (void*)elasticdata->value.addr, 
		    DEFAULT_STR_VALUE, 
		    elasticdata->value.size);
	}
#ifdef OUTPUT_HASH_KEYS
	print = snprintf( printf_buffer, 
			  sizeof(printf_buffer), 
			  "\n[%6d]=0x%8s, ", 
			  map_buffer->header.count,
			  PrintableHash(hashstr, &elasticdata->key_hash, HASH_SIZE )
			    );
	bio->write(bio, STDOUT, printf_buffer, print);
	bio->write(bio, STDOUT, 
		   (void*)elasticdata->key_data.addr, elasticdata->key_data.size);
#endif
    }
#ifdef OUTPUT_HASH_KEYS
    bio->flush_write(bio,STDOUT);
    free(bio->data.buf);
    free(bio);
#endif
    /*return real handled data pos*/
    return current_pos;
}

#define COMBINE_VALUES(newelasticdata, elasticdata2)			\
    /*save value depends on wordcount config*/				\
    if ( VALUE_ADDR_AS_DATA == 1){					\
	/*use combine_elasticdata->value.addr as 4byte value*/		\
	(newelasticdata)->value.addr += (elasticdata2)->value.addr;	\
	(newelasticdata)->own_value = EDataNotOwned;			\
    }									\
    else{								\
	/*interpret elasticdata->value.addr as 4bytes pointer		\
	 *and save non null terminated string representation of value;	\
	 *it's test second use case of mapreduce library*/		\
	(newelasticdata)->value.addr					\
	    = (uintptr_t) realloc( (void*)(newelasticdata)->value.addr,	\
				   (newelasticdata)->value.size		\
				   + (elasticdata2)->value.size + 1 );	\
	/*add "+" char*/						\
	memcpy( (void*)(newelasticdata)->value.addr+(newelasticdata)->value.size, \
		"+", 1 );						\
	/*add value*/							\
	memcpy( (void*)(newelasticdata)->value.addr+(newelasticdata)->value.size+1, \
		(void*)(elasticdata2)->value.addr,			\
		(elasticdata2)->value.size );				\
	(newelasticdata)->value.size += (elasticdata2)->value.size+1;	\
    }



/*******************************************************************************
 * User combine for MAP REDUCE*/
int Combine( const Buffer *map_buffer, 
	     Buffer *reduce_buffer ){
    int combined_count=0;

    /*declare buf item to use it as current loop item*/
    ElasticBufItemData* current_elasticdata;
    /*declare combine item and init it default by nearest value */
    ElasticBufItemData* combine_elasticdata = alloca(map_buffer->header.item_size);
    if ( map_buffer->header.count ){
	GetBufferItem( map_buffer, 0, combine_elasticdata );
    }

    /*go through items of sorted map_buffer*/
    for (int i=0; i < map_buffer->header.count; i++){
	/*current loop item*/
	current_elasticdata = (ElasticBufItemData*) BufferItemPointer( map_buffer, i );
	/*if current item can be added into reduce_buffer*/
	if ( //(i == map_buffer->header.count - 1) ||
	    *(HASH_TYPE*)&current_elasticdata->key_hash != 
	    *(HASH_TYPE*)&combine_elasticdata->key_hash /* && i > 0 */ ){

 	    /*save previously combined item*/
	    AddBufferItem( reduce_buffer, combine_elasticdata );
	    /*set current item as next combine value */
	    GetBufferItem( map_buffer, i, combine_elasticdata );
	    combined_count=1;
	}
	/*if previous item and new one has the same key then we need 
	 *to modify value of item to reduce it*/
	else{
	    ++combined_count;
	    /*update value only for items that now combining more than one item*/
	    if ( combined_count > 1 ){
		COMBINE_VALUES(combine_elasticdata, current_elasticdata);
		TRY_FREE_MRITEM_DATA(current_elasticdata);
	    }
	}
    }

    /*save last combined value if combined item not yet added*/
    if (combined_count>0){
	AddBufferItem( reduce_buffer, combine_elasticdata );
    }
    return 0;
}

int Reduce( const Buffer *reduced_buffer ){
    BufferedIOWrite* bio = AllocBufferedIOWrite( malloc(IO_BUF_SIZE), IO_BUF_SIZE);
    WRITE_FMT_LOG("bio->data.buf=%p\n", bio->data.buf);

#define PRINT_POS bio->data.buf+bio->data.cursor
#define PRINT_MAX bio->data.bufmax-bio->data.cursor
#define PRINT_POS_INC(inc) bio->data.cursor+=(inc)

    /*declare buf item to use it as current loop item*/
    ElasticBufItemData* elasticdata;
    HASH_TYPE prev_key=0;
    for ( int i=0; i < reduced_buffer->header.count; i++ ){
	elasticdata = (ElasticBufItemData*)BufferItemPointer( reduced_buffer, i );

	/*flush buffer if can't be added into buffer*/
	if ( HASH_STR_LEN > PRINT_MAX ) bio->flush_write(bio, STDOUT);
	/*copy text hash direct into memory & increment buffered pos*/
	PRINT_POS_INC( strlen(PrintableHash(PRINT_POS, &elasticdata->key_hash, HASH_SIZE))); 
	/*copy helper chars*/
#define FORMAT_1 " ["
	bio->write(bio, STDOUT, FORMAT_1, strlen(FORMAT_1));
	/*write key, doing buffer size check*/
	bio->write(bio, STDOUT, (void*)elasticdata->key_data.addr, elasticdata->key_data.size);
	if ( VALUE_ADDR_AS_DATA == 1){
	    /*value.addr as data */
	    if ( HASH_STR_LEN+10 > PRINT_MAX ) bio->flush_write(bio, STDOUT);
	    PRINT_POS_INC( snprintf( PRINT_POS, PRINT_MAX, "]=%d\n", elasticdata->value.addr ));
	}
	else{
#define FORMAT_2 "]="
	    bio->write(bio, STDOUT, FORMAT_2, strlen(FORMAT_2));
	    bio->write(bio, STDOUT, (void*)elasticdata->value.addr, elasticdata->value.size );
	    bio->write(bio, STDOUT, "\n", 1);
	}

	/*test data sorted by hash*/
	HASH_TYPE* keyhash = (HASH_TYPE*)&elasticdata->key_hash;
	if ( i>0 && prev_key >= *keyhash ){
	    bio->flush_write(bio, STDOUT);
	    printf("test failed prev_key=%s, key=%s\n", 
		   PrintableHash(alloca(HASH_STR_LEN), (const uint8_t *)&prev_key, HASH_SIZE),
		   PrintableHash(alloca(HASH_STR_LEN), (const uint8_t *)keyhash, HASH_SIZE) );
	    fflush(0);
	    exit(-1);
	}

	prev_key = (HASH_TYPE)elasticdata->key_hash;
	TRY_FREE_MRITEM_DATA(elasticdata);  

    }
    bio->flush_write(bio, STDOUT);

    WRITE_FMT_LOG("bio->data.buf=%p\n", bio->data.buf);
    free(bio->data.buf);     /*free buffer in this way because not saved pointer*/
    WRITE_LOG("OK");
    free(bio);
    return 0;
}


void InitInterface( struct MapReduceUserIf* mr_if ){
    memset( mr_if, '\0', sizeof(struct MapReduceUserIf) );
    PREPARE_MAPREDUCE(mr_if, 
		      Map, 
		      Combine, 
		      Reduce, 
		      ComparatorElasticBufItemByHashQSort,
		      ComparatorHash,
		      PrintableHash,
		      VALUE_ADDR_AS_DATA,
		      /*provide size of user structure. 
			we assume key_hash size sizeof(HASH_TYPE)*/
		      sizeof(
			     struct{
				 BinaryData     key_data;
				 BinaryData     value; 
				 uint8_t        own_key; 
				 uint8_t        own_value; 
				 HASH_TYPE      key_hash; 
			     }),
		      HASH_SIZE );
}


