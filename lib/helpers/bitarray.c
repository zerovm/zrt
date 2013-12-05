
#include "bitarray.h"

#include <assert.h>


static int bitarray_search_emptybit_sequence_begin(struct BitArray* this, 
					  int begin_offset,
					  int len_of_sequence){
    int i, j, search_seq_len=0;
    assert(len_of_sequence);
    for ( i=0; i < this->array_size; i++ ){
	/*if at least one bit non setted, then locate it*/
	if ( this->array[i] != 0xFF ){
	    for ( j=0; j < 8 && search_seq_len < len_of_sequence; j++ ){
		if ( !this->public.get_bit(this, i*8+j) )
		    search_seq_len++;
		else
		    search_seq_len=0;
	    }

	    /*if search complete*/
	    if ( search_seq_len == len_of_sequence ){
		/*update default search pos*/
		this->array_search_pos = i;
		int matched_index = i*8+(j-1);
		return matched_index-len_of_sequence+1;
	    }
	}
	else{
	    search_seq_len = 0;
	}
    }
    return -1;
}


/*set bit & flush cache*/
static void bitarray_toggle_bit(struct BitArray* this, int index){
    /*toggle bit*/
    this->array[index/8] ^= 1 << (index % 8);
    /*if new bit value is 0 then update search pos*/
    if ( !this->public.get_bit(this, index) && index < this->array_search_pos )
	this->array_search_pos = index/8;
}

static unsigned char bitarray_get_bit(struct BitArray* this, int index){
    return 1 & (this->array[index/8] >> (index % 8));
}

/*
  implem must be NULL if want to alloc it in heap, or to use existing
  object provide pointer */
struct BitArrayPublicInterface* bit_array_construct( unsigned char* array, int array_size, struct BitArray* exist ){
    /*use existing object memory, for example resided in bss  */
    struct BitArray* this = exist;

    /*set functions*/
    this->public.toggle_bit = bitarray_toggle_bit;
    this->public.get_bit =    bitarray_get_bit;
    this->public.search_emptybit_sequence_begin 
	= bitarray_search_emptybit_sequence_begin;
    /*set data members*/
    this->array_size = array_size;
    this->array = array;
    this->array_search_pos = 0;
    return (struct BitArrayPublicInterface*)this;
}

