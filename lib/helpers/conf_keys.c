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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include "zrtlog.h"
#include "conf_keys.h"

/*****************************************
 * keys of params related to single record
 *****************************************/


static int add_key_to_list(struct KeyList* list, const char* key){
    assert(list);
    /*copy key up to maximum length*/
    strncpy( list->keys[list->count], key, NVRAM_MAX_KEY_LENGTH );
    ZRT_LOG(L_INFO, "%s", key);
    /*if folowing assert is raised then just increase NVRAM_MAX_KEYS_COUNT_IN_RECORD value*/
    assert(list->count<NVRAM_MAX_KEYS_COUNT_IN_RECORD);
    return list->count++; /*get index of added key*/
}

static int key_find(const struct KeyList* list, const char* key, int keylen){
    assert(list);
    int i;
    for(i=0; i < list->count; i++ ){
	/*compare key ignoring case*/
	int etalonkeylen = strlen(list->keys[i]);
	if ( !strncmp(key, list->keys[i], etalonkeylen) &&
	     keylen == etalonkeylen )
	    {
		return i; /*get key index, specified key macthed*/
	    }
    }
    return -1; /*wrong key*/
}

/*****
 construction */

void keys_construct(struct KeyList* keys){
    assert(keys);
    keys->count = 0;
    keys->add_key = add_key_to_list;
    keys->find = key_find;
}

