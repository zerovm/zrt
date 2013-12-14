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



#ifndef __ELASTIC_MR_ITEM_H__
#define __ELASTIC_MR_ITEM_H__

#include <stdint.h>

enum { EDataNotOwned=0, EDataOwned=1 };

/*if data owned it will be freed, else nothing happens*/
#define TRY_FREE_MRITEM_DATA(elasticitem_p)				\
    if ( (elasticitem_p)->own_value == EDataOwned )			\
	free((void*)(elasticitem_p)->value.addr), (elasticitem_p)->value.addr=0; \
    else if ( (elasticitem_p)->own_key == EDataOwned )			\
	free((void*)(elasticitem_p)->key_data.addr), (elasticitem_p)->key_data.addr=0;

typedef struct BinaryData{
    uintptr_t addr;
    int       size;
} BinaryData;

/*sizeof must not be used, struct hash variable length*/
typedef struct ElasticBufItemData{
    BinaryData     key_data;
    /*value.addr also may be interpreted as data depending of MR configuration,
     in this case value.size will not be used*/
    BinaryData     value; 
    /*1-owned or 0-not value referred by pointer, for owned values they should be freed 
      by user functions.*/
    uint8_t        own_key; 
    /*1-owned or 0-not key referred by pointer, for owned keys they should be freed 
      by user functions.*/
    uint8_t        own_value; 
    /*key_hash not strictly equal to 1byte, it can out of buond of declared structure
      because data type size can be configured at runtime, so real size of whole struct 
      will vary too. don't change this data type size, we assume it has 1byte size.*/
    uint8_t        key_hash; 
} ElasticBufItemData;


#endif //__ELASTIC_MR_ITEM_H__
