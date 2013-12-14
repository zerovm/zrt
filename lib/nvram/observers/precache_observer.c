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
#include <assert.h>

#include "zrt_defines.h"

#include "zrtlog.h"
#include "precache_observer.h"
#include "nvram.h"
#include "conf_parser.h"
#include "conf_keys.h"


#define PRECACHE_PARAM_PRECACHE_KEY_INDEX    0

static struct MNvramObserver s_precache_observer;

void handle_precache_record(struct MNvramObserver* observer,
			 struct ParsedRecord* record,
			 void* obj1, void* obj2, void* obj3){
    assert(record);
    /*get param*/
    char* precache = NULL;
    int* dofork = (int*)obj1;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[PRECACHE_PARAM_PRECACHE_KEY_INDEX], 
		       &precache);
    ZRT_LOG(L_SHORT, "precache record: precache=%s", precache);

    if ( precache ){
	if ( !strcmp("yes", precache) ){
	    *dofork = 1;
	}
	else{
	    *dofork = 0;
	}
    }
}

struct MNvramObserver* get_precache_observer(){
    struct MNvramObserver* self = &s_precache_observer;
    ZRT_LOG(L_INFO, "Create observer for section: %s", PRECACHE_SECTION_NAME);
    /*setup section name*/
    strncpy(self->observed_section_name, PRECACHE_SECTION_NAME, NVRAM_MAX_SECTION_NAME_LEN);
    /*setup section keys*/
    keys_construct(&self->keys);
    /*add keys and check returned key indexes that are the same as expected*/
    int key_index;
    /*check parameters*/
    key_index = self->keys.add_key(&self->keys, PRECACHE_PARAM_PRECACHE_KEY);
    assert(PRECACHE_PARAM_PRECACHE_KEY_INDEX==key_index);

    /*setup functions*/
    s_precache_observer.handle_nvram_record = handle_precache_record;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", PRECACHE_SECTION_NAME);
    return &s_precache_observer;
}

