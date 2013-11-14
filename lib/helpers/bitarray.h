#ifndef __BITARRAY_H__
#define __BITARRAY_H__

struct BitArrayImplem{
    void (*toggle_bit)(struct BitArrayImplem* this, int index);
    char (*get_bit)   (struct BitArrayImplem* this, int index);
    /*@return starting bit index, or -1 if no empty bits located*/
    int  (*search_emptybit_sequence_begin)(struct BitArrayImplem* this, int len_of_sequence);
    //data
    int array_size;
    unsigned char* array;
    /*search_pos is using to accelerate search by skipping setted bits*/
    int array_search_pos;
};

void init_bit_array( struct BitArrayImplem* implem, unsigned char* array, int array_size );

#endif //__BITARRAY_H__

