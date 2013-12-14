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
#include <limits.h>
#include <assert.h>

#include "zrt_defines.h"

#include "zrtlog.h"
#include "settime_observer.h"
#include "utils.h"
#include "nvram.h"
#include "conf_parser.h"
#include "conf_keys.h"


#define TIME_PARAM_SECONDS_KEY_INDEX    0

static struct MNvramObserver s_settime_observer;

void handle_time_record(struct MNvramObserver* observer,
			struct ParsedRecord* record,
			void* obj1, void* obj2, void* obj3){
    assert(record);

    /*obj1 - only channels filesystem interface*/
    struct timeval* time_cache = (struct timeval*)obj1;

    /*get param seconds*/
    char* seconds = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[TIME_PARAM_SECONDS_KEY_INDEX], 
		       &seconds);
    ZRT_LOG(L_SHORT, "time record: seconds=%s", seconds);

    /* Setup system time
       seconds since 1970-01-01 00:00:00 UTC
       to get time value in terminal: date +"%s"
       get time stamp from the environment, and cache it */
    if ( seconds != NULL ){
	int err;
        time_cache->tv_usec = 0; /* msec not supported */
	time_cache->tv_sec = strtouint_nolocale(seconds, 10, &err );
        ZRT_LOG(L_SHORT, "time_cache->tv_sec=%lld", time_cache->tv_sec );
    }
}

struct MNvramObserver* get_settime_observer(){
    struct MNvramObserver* self = &s_settime_observer;
    ZRT_LOG(L_INFO, "Create observer for section: %s", TIME_SECTION_NAME);
    /*setup section name*/
    strncpy(self->observed_section_name, TIME_SECTION_NAME, NVRAM_MAX_SECTION_NAME_LEN);
    /*setup section keys*/
    keys_construct(&self->keys);
    /*add keys and check returned key indexes that are the same as expected*/
    int key_index;
    /*check parameters*/
    key_index = self->keys.add_key(&self->keys, TIME_PARAM_SECONDS_KEY);
    assert(TIME_PARAM_SECONDS_KEY_INDEX==key_index);

    /*setup functions*/
    s_settime_observer.handle_nvram_record = handle_time_record;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", TIME_SECTION_NAME);
    return &s_settime_observer;
}

