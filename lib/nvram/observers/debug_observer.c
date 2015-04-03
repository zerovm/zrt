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
#include "debug_observer.h"
#include "nvram.h"
#include "conf_parser.h"
#include "conf_keys.h"


#define DEBUG_PARAM_VERBOSITY_KEY_INDEX    0

static struct MNvramObserver s_debug_observer;

void handle_debug_record(struct MNvramObserver* observer,
			 struct ParsedRecord* record,
			 void* obj1, void* obj2, void* obj3){
    assert(record);
    /*get param*/
    char* verbosity = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[DEBUG_PARAM_VERBOSITY_KEY_INDEX], 
		       &verbosity);
    ZRT_LOG(L_SHORT, "debug record: verbosity=%s", verbosity);

    if ( verbosity ){
	char v = verbosity[0];
	/*simplest retrieving of single digit*/
	if ( v >= '0' && v <= '9' ){
	    ZRT_LOG(L_BASE, "verbosity=%d", v - '0' );
	    __zrt_log_set_verbosity( v - '0' );
	}
    }
}

struct MNvramObserver* get_debug_observer(){
    struct MNvramObserver* self = &s_debug_observer;
    ZRT_LOG(L_INFO, "Create observer for section: %s", DEBUG_SECTION_NAME);
    /*setup section name*/
    strncpy(self->observed_section_name, DEBUG_SECTION_NAME, NVRAM_MAX_SECTION_NAME_LEN);
    /*setup section keys*/
    keys_construct(&self->keys);
    /*add keys and check returned key indexes that are the same as expected*/
    int key_index;
    /*check parameters*/
    key_index = self->keys.add_key(&self->keys, DEBUG_PARAM_VERBOSITY_KEY, NULL);
    assert(DEBUG_PARAM_VERBOSITY_KEY_INDEX==key_index);

    /*setup functions*/
    s_debug_observer.handle_nvram_record = handle_debug_record;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", DEBUG_SECTION_NAME);
    return &s_debug_observer;
}

