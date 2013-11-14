
#include "bitarray.h"

#include <assert.h>

static int search_emptybit_sequence_begin(struct BitArrayImplem* this, int len_of_sequence){
    int i, j, search_seq_len;
    assert(len_of_sequence);
    for ( i=0; i < this->array_size; i++ ){
	/*if at least one bit non setted, then locate it*/
	if ( this->array[i] != 0xFF ){
	    search_seq_len = 0;
	    for ( j=i*8; j < this->array_size*8 && search_seq_len < len_of_sequence; j++ ){
		if ( !this->get_bit(this, j) )
		    search_seq_len++;
		else
		    search_seq_len=0;
	    }
	    /*if search complete*/
	    if ( search_seq_len == len_of_sequence ){
		int matched_index = j-1;
		/*update default search pos*/
		this->array_search_pos = matched_index/8;
		return matched_index-len_of_sequence+1;
	    }
	}
    }
    return -1;
}

/*set bit & flush cache*/
static void toggle_bit(struct BitArrayImplem* this, int index){
    /*toggle bit*/
    this->array[index/8] ^= 1 << (index % 8);
    /*if new bit value is 0 then update search pos*/
    if ( !this->get_bit(this, index) && index < this->array_search_pos )
	this->array_search_pos = index/8;
}

static unsigned char get_bit(struct BitArrayImplem* this, int index){
    return 1 & (this->array[index/8] >> (index % 8));
}

void init_bit_array( struct BitArrayImplem* implem, unsigned char* array, int array_size ){
    /*set functions*/
    implem->toggle_bit = toggle_bit;
    implem->get_bit = get_bit;
    implem->search_emptybit_sequence_begin = search_emptybit_sequence_begin;
    /*set data members*/
    implem->array_size = array_size;
    implem->array = array;
    implem->array_search_pos = 0;
}

