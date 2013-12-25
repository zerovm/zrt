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


#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "zrt_defines.h"

#include "zrtlog.h"
#include "mapping_observer.h"
#include "nvram.h"
#include "conf_parser.h"
#include "conf_keys.h"
#include "channels_mount.h"


#define MAPPING_PARAM_CHANNEL_KEY_INDEX    0
#define MAPPING_PARAM_TYPE_KEY_INDEX       1

static struct MNvramObserver s_mapping_observer;
static struct ChannelsModeUpdaterPublicInterface *s_nvram_mode_setting_updater;

void handle_mapping_record(struct MNvramObserver* observer,
			   struct ParsedRecord* record,
			   void* obj1, void* obj2, void* obj3){
    assert(record);

    /*get param */
    char* channel = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[MAPPING_PARAM_CHANNEL_KEY_INDEX], 
		       &channel);
    /*get param */
    char* mode = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[MAPPING_PARAM_TYPE_KEY_INDEX], 
		       &mode);

    ZRT_LOG(L_SHORT, "mapping record: channel=%s, mode=%s", channel, mode);

    if ( channel != NULL && mode != NULL ){
	int channel_mode=-1;
	if (!strcmp(mode, MAPPING_PARAM_VALUE_PIPE))
	    channel_mode = S_IFIFO;
	else if (!strcmp(mode, MAPPING_PARAM_VALUE_CHR))
	    channel_mode = S_IFCHR;
	else if (!strcmp(mode, MAPPING_PARAM_VALUE_FILE))
	    channel_mode = S_IFREG;

	assert(s_nvram_mode_setting_updater);

	if ( channel_mode > 0 ){
	    s_nvram_mode_setting_updater->set_channel_mode(s_nvram_mode_setting_updater, 
							   channel, channel_mode);
	    ZRT_LOG(L_BASE, "channel=%s, mode=octal %o", channel, channel_mode );
	}
    }
}

void set_mapping_channels_settings_updater( struct ChannelsModeUpdaterPublicInterface 
					    *nvram_mode_setting_updater ){
    s_nvram_mode_setting_updater = nvram_mode_setting_updater;
}


struct MNvramObserver* get_mapping_observer(){
    struct MNvramObserver* self = &s_mapping_observer;
    ZRT_LOG(L_INFO, "Create observer for section: %s", MAPPING_SECTION_NAME);
    /*setup section name*/
    strncpy(self->observed_section_name, MAPPING_SECTION_NAME, NVRAM_MAX_SECTION_NAME_LEN);
    /*setup section keys*/
    keys_construct(&self->keys);
    /*add keys and check returned key indexes that are the same as expected*/
    int key_index;
    /*check parameters*/
    key_index = self->keys.add_key(&self->keys, MAPPING_PARAM_CHANNEL_KEY);
    assert(MAPPING_PARAM_CHANNEL_KEY_INDEX==key_index);
    /*check parameters*/
    key_index = self->keys.add_key(&self->keys, MAPPING_PARAM_TYPE_KEY);
    assert(MAPPING_PARAM_TYPE_KEY_INDEX==key_index);

    /*setup functions*/
    s_mapping_observer.handle_nvram_record = handle_mapping_record;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", MAPPING_SECTION_NAME);
    return &s_mapping_observer;
}

