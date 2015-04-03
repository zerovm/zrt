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

#include "zrtlog.h"
#include "environment_observer.h"
#include "nvram_loader.h"
#include "nvram.h"
#include "conf_parser.h"
#include "conf_keys.h"


#define ENV_PARAM_NAME_KEY_INDEX    0
#define ENV_PARAM_VALUE_KEY_INDEX   1

static struct MNvramObserver s_env_observer;

static void add_pair_to_temp_buffer(char* buf, int bufsize, int* index,
				    const char* name, int len, const char* val, int len2){
    /*add env pairs into buffer, every pair end must be null term char '\0' */
    if ( *index+len+len2 < bufsize ){
	memcpy(buf+*index, name, len);
	(*index)+=len;
	buf[ (*index)++ ] = '=';
	/*copy value contets*/
	memcpy(buf+*index, val, len2);
	*index += len2;
	buf[ (*index)++ ] = '\0';
    }
    else{
	ZRT_LOG(L_BASE, "can't save env %s=%s, insufficient buffer size=%d/%d",
		name, val, *index+len+len2+1, bufsize );
    }
}

void get_env_array(char **envs, char* buf, int bufsize){
    int idx=0;
    int handled_buf_idx=0;
    int i;
    while( idx < NVRAM_MAX_RECORDS_IN_SECTION ){
	for(i=handled_buf_idx; i < bufsize && idx < NVRAM_MAX_RECORDS_IN_SECTION; i++ ){
	    if ( buf[i] == '\0' ){
		envs[idx++] = &buf[handled_buf_idx];
		handled_buf_idx = i+1;
		continue;
	    }
	}
	/*last item NULL pointer*/
	envs[idx] = NULL;
	break;
    }
}

/*interface function
 while handling data it's saving it to buffer provided in obj1 and updating 
 used buffer space (index) in obj3 param*/
void handle_env_record(struct MNvramObserver* observer,
		       struct ParsedRecord* record,
		       void* obj1, void* obj2, void* obj3){
    assert(record);
    /*get param*/
    char* envname = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[ENV_PARAM_NAME_KEY_INDEX], 
		      &envname);
    /*get param*/
    char* envval = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[ENV_PARAM_VALUE_KEY_INDEX], 
		      &envval);
    ZRT_LOG(L_INFO, "env record: %s=%s", envname, envval);
    /*unescape value*/
    char* unescaped_value_result = alloca( strlen(envval)+1 );
    int unescaped_value_len =
	unescape_string_copy_to_dest(envval, 
				     record->parsed_params_array[ENV_PARAM_VALUE_KEY_INDEX].vallen, 
				     unescaped_value_result);
    ZRT_LOG(L_SHORT, "env record: %s=%s (escaped) %d", envname, unescaped_value_result, unescaped_value_len);

    /*If handling envs at session start preparing main() arguments,
     it's expected code running at prolog stage*/
    if ( obj1 && obj2 && obj3 ){
	char* buffer = (char*)obj1; /*obj1 - char* */
	int bufsize = *(int*)obj2;  /*obj2 - int*  */
	int* index = (int*)obj3;    /*obj3 - int*  */
	assert(buffer); /*into buffer will be saved results*/

	add_pair_to_temp_buffer(buffer, bufsize, index,
				envname, 
				record->parsed_params_array[ENV_PARAM_NAME_KEY_INDEX].vallen, 
				unescaped_value_result, 
				unescaped_value_len);
    }
    /*Handling envs at forked session, no another input parameters needed */
    else{
	if ( setenv(envname, unescaped_value_result, 1) == 0 ){
	    ZRT_LOG(L_INFO, P_TEXT, "env overwrite success");
	}
	else{
	    ZRT_LOG(L_INFO, P_TEXT, "env overwrite failed");
	}
    }
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
    key_index = self->keys.add_key(&self->keys, ENVIRONMENT_PARAM_NAME_KEY, NULL);
    assert(ENV_PARAM_NAME_KEY_INDEX==key_index);
    key_index = self->keys.add_key(&self->keys, ENVIRONMENT_PARAM_VALUE_KEY, NULL);
    assert(ENV_PARAM_VALUE_KEY_INDEX==key_index);

    /*setup functions*/
    s_env_observer.handle_nvram_record = handle_env_record;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", ENVIRONMENT_SECTION_NAME);
    return &s_env_observer;
}


