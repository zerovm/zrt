/*
 *
 * Copyright (c) 2013, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __BITARRAY_H__
#define __BITARRAY_H__

#include "zrt_defines.h"

/*name of constructor*/
#define BIT_ARRAY bit_array_construct 

struct BitArray;

struct BitArrayPublicInterface{
    void (*toggle_bit)(struct BitArrayPublicInterface*this, int index);
    char (*get_bit)   (struct BitArrayPublicInterface* this, int index);
    /*@return starting bit index, or -1 if no empty bits located*/
    int  (*search_emptybit_sequence_begin)(struct BitArrayPublicInterface* this, int begin_offset, int len_of_sequence);
};


/*all static variables moved into subclass, it is an analog of private
  members.  */
struct BitArray{
    //base, it is must be a first member
    struct BitArrayPublicInterface public;
    /*private funcs*/
    int (*test)();
    /*private data*/
    int array_size;
    unsigned char* array;
    /*search_pos is using to accelerate search by skipping setted bits*/
    int array_search_pos;
};


/*@return result pointer can be casted to struct BitArray*/
struct BitArrayPublicInterface* 
bit_array_construct( unsigned char* array, int array_size, struct BitArray* implem );

#endif //__BITARRAY_H__

