/*
 * environment_observer.c
 *
 *  Created on: 22.04.2013
 *      Author: yaroslav
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "zrtlog.h"
#include "environment_observer.h"
#include "nvram_loader.h"
#include "nvram.h"
#include "conf_parser.h"
#include "conf_keys.h"


#define ENVIRONMENT_PARAM_NAME_KEY_INDEX    0
#define ENVIRONMENT_PARAM_VALUE_KEY_INDEX   1

static struct MNvramObserver s_env_observer;

void handle_env_record(struct MNvramObserver* observer,
		       struct NvramLoader* nvram_loader,
		       struct ParsedRecord* record){
    /*get param*/
    char* envname = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[ENVIRONMENT_PARAM_NAME_KEY], 
		      &envname);
    /*get param*/
    char* envval = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[ENVIRONMENT_PARAM_VALUE_KEY], 
		      &envval);
    ZRT_LOG(L_SHORT, "env record: %s=%s", envname, envval);
    ZRT_LOG_DELIMETER;
}

static void cleanup_env_observer( struct MNvramObserver* obs){
}

struct MNvramObserver* get_env_observer(){
    struct MNvramObserver* self = &s_env_observer;
    ZRT_LOG(L_INFO, "Create observer for section: %s", ENVIRONMENT_SECTION_NAME);
    /*setup section name*/
    strncpy(self->observed_section_name, ENVIRONMENT_SECTION_NAME, NVRAM_MAX_SECTION_NAME_LEN);
    /*setup section keys*/
    keys_construct(&self->keys);
    /*add keys and check returned key indexes that are the same as expected*/
    int key_index;
    /*check parameters*/
    key_index = self->keys.add_key(&self->keys, ENVIRONMENT_PARAM_NAME_KEY);
    assert(ENVIRONMENT_PARAM_NAME_KEY_INDEX==key_index);
    key_index = self->keys.add_key(&self->keys, ENVIRONMENT_PARAM_VALUE_KEY);
    assert(ENVIRONMENT_PARAM_VALUE_KEY_INDEX==key_index);

    /*setup functions*/
    s_fstat_observer.handle_nvram_record = handle_env_record;
    s_fstat_observer.cleanup = cleanup_env_observer;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", ENVIRONMENT_SECTION_NAME);
    return &s_fstat_observer;
}

