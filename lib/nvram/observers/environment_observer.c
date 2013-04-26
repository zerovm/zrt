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


#define ENV_PARAM_NAME_KEY_INDEX    0
#define ENV_PARAM_VALUE_KEY_INDEX   1

static struct MNvramObserver s_env_observer;
static char* s_temp_buffer;
static int   s_temp_bufsize;
static int   s_temp_bufindex;

static void add_pair_to_temp_buffer(const char* name, int len, const char* val, int len2){
    if ( s_temp_bufindex+len+len2 < s_temp_bufsize ){
	memcpy(s_temp_buffer+s_temp_bufindex, name, len);
	s_temp_bufindex+=len;
	s_temp_buffer[s_temp_bufindex++] = '=';
	memcpy(s_temp_buffer+s_temp_bufindex, val, len2);
	s_temp_buffer[s_temp_bufindex++] = '\0';
    }
    else{
	ZRT_LOG(L_BASE, "can't save env %s=%s, insufficient buffer size=%d/%d",
		name, val, s_temp_bufindex+len+len2+1, s_temp_bufsize );
    }
}

void set_environments_buffer(char* buffer, int bufsize){
    assert(buffer);
    s_temp_buffer = buffer;
    s_temp_bufsize = bufsize;
    s_temp_bufindex = 0;
}

void get_env_array(char **envs){
    int idx=0;
    int handled_buf_idx=0;
    int i;
    while( idx < NVRAM_MAX_RECORDS_IN_SECTION ){
	for(i=handled_buf_idx; i < s_temp_bufindex; i++ ){
	    if ( s_temp_buffer[i] == '\0' ){
		envs[idx++] = &s_temp_buffer[handled_buf_idx];
		handled_buf_idx = i+1;
		continue;
	    }
	}
	/*last item NULL pointer*/
	envs[idx] = NULL;
	break;
    }
}

/*interface functions*/
void handle_env_record(struct MNvramObserver* observer,
		       struct ParsedRecord* record,
		       void* obj1, void* obj2){
    assert(record);
    /*get param*/
    char* envname = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[ENV_PARAM_NAME_KEY_INDEX], 
		      &envname);
    /*get param*/
    char* envval = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[ENV_PARAM_VALUE_KEY_INDEX], 
		      &envval);
    ZRT_LOG(L_SHORT, "env record: %s=%s", envname, envval);
    add_pair_to_temp_buffer(envname, 
			    record->parsed_params_array[ENV_PARAM_NAME_KEY_INDEX].vallen, 
			    envval, 
			    record->parsed_params_array[ENV_PARAM_VALUE_KEY_INDEX].vallen);
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
    assert(ENV_PARAM_NAME_KEY_INDEX==key_index);
    key_index = self->keys.add_key(&self->keys, ENVIRONMENT_PARAM_VALUE_KEY);
    assert(ENV_PARAM_VALUE_KEY_INDEX==key_index);

    /*setup functions*/
    s_env_observer.handle_nvram_record = handle_env_record;
    s_env_observer.cleanup = cleanup_env_observer;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", ENVIRONMENT_SECTION_NAME);
    return &s_env_observer;
}


