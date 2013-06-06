/*
 * mapping_observer.c
 *
 *  Created on: 6.06.2013
 *      Author: yaroslav
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
	int t=-1;
	if (!strcmp(mode, MAPPING_PARAM_VALUE_PIPE))
	    t = S_IFIFO;
	else if (!strcmp(mode, MAPPING_PARAM_VALUE_CHR))
	    t = S_IFCHR;
	else if (!strcmp(mode, MAPPING_PARAM_VALUE_FILE))
	    t = S_IFREG;

	uint* cmode;
	if ( t > 0 && (cmode=channel_mode(channel)) != NULL ){
	    *cmode = t;
	    ZRT_LOG(L_BASE, "channel=%s, mode=octal %o", channel, *cmode );
	}
    }
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

