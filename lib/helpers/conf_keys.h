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

#ifndef CONF_KEYS_H
#define CONF_KEYS_H

#include "nvram.h"

/*Expected keys for single record in config file. */
struct KeyList{
    //functions
    /*Add key and default value to keys_list, this just saves a
     *pointer, it is expected that values are stored in BSS 
     @param key
     *@param optional_default_value not NULL if param is optional then
     *assign default value*/
    int (*add_key)(struct KeyList* list, const char* key, const char* optional_default_value);
    /*@return index of key in array if specified key is found, -1 if not*/
    int (*find)(const struct KeyList* list, const char* key, int keylen);
    //preallocated space for keys
    char *keys[NVRAM_MAX_KEYS_COUNT_IN_RECORD];
    /*preallocated space for values of optinal keys*/
    char *optional_default_values[NVRAM_MAX_KEYS_COUNT_IN_RECORD];
    int  count;
};

/*assign functions pointers for keys struct and return it
 *@param keys to construct*/
void keys_construct(struct KeyList* keys);

#endif //CONF_KEYS_H
