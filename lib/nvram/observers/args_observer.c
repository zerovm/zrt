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

#define _GNU_SOURCE 
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "zrtlog.h"
#include "args_observer.h"
#include "nvram_loader.h"
#include "nvram.h"
#include "conf_parser.h"
#include "conf_keys.h"


#define ARG_PARAM_VALUE_KEY_INDEX   0

static struct MNvramObserver s_arg_observer;

/*parsed arg, save it*/
#define SAVE_PARSED_ARG(records, r_array_len, r_count, parsed)	\
    if ( r_count < r_array_len ){				\
	records[(r_count)++] = parsed;				\
	(parsed).val = NULL, (parsed).vallen=0;			\
    }
    

/*Read args_buf and add every string literal separated by spaces and
  tabs into dedicated array item from parsed_args_array, also escaped
  data will be unescaped and unnecessary config artefatcs will be
  pruned.*/
static int parse_config_args(struct ParsedParam* parsed_args_array, 
			     int args_array_len,
			     const char* args_buf, int bufsize){
    struct ParsedParam temp = {0, NULL, 0};
    int count=0;
    int index=0;

    while( index < bufsize ){
	if ( args_buf[index] == ' ' || 
	     args_buf[index] == '\t' ){
	    if ( temp.val != NULL ){
		temp.vallen = &args_buf[index] - temp.val;
		SAVE_PARSED_ARG(parsed_args_array, args_array_len, count, temp);
	    }
	}
	/*located " after space, skipped if not preceded by space */
	else if ( args_buf[index] == '"' && temp.val == NULL ){
	    char* double_quote=NULL;
	    if ( index+1 < bufsize && (double_quote=strchrnul(args_buf+index+1, '"')) != NULL ){
		/*extract value between double quotes*/
		temp.val = (char*)&args_buf[index+1];
		temp.vallen = double_quote - temp.val;
		index += temp.vallen +1;
		SAVE_PARSED_ARG(parsed_args_array, args_array_len, count, temp);
	    }
	    else{
		temp.val = (char*)&args_buf[index];
	    }
	}
	else if ( temp.val == NULL ){
	    temp.val = (char*)&args_buf[index];
	}
	++index;
    }

    //save last parsed data
    if ( temp.val != NULL ){
	temp.vallen = &args_buf[index] - temp.val;
	SAVE_PARSED_ARG(parsed_args_array, args_array_len, count, temp);
    }
    return count;
}


/*Source data from config can consist from escaped characters and also
  some elements can be surrounded by double quotes, all of it
  transforms into clear data without config artefacts;
@param dest_buf
@param bufsize size of dest_buf 
@param index this is pointer to get the length of data in buffer 
@param source_buf array of characters with source data 
@param source_buf_len length of source data*/
static void parse_transform_and_place_into_buf(char* dest_buf, int bufsize, int* index,
					       const char* source_buf, int source_buf_len){
    /*add arg pairs into buffer, every pair end must be null term char '\0' */
    if ( *index+source_buf_len < bufsize ){
	/*all arguments coming unparsed, parse it here*/
	struct ParsedParam args[NVRAM_MAX_RECORDS_IN_SECTION];
	/*in case if too many args will be parsed they will be skipped*/
	int argc = parse_config_args(args, NVRAM_MAX_RECORDS_IN_SECTION, source_buf, source_buf_len);
	ZRT_LOG(L_BASE, "argc= %d", argc );
	int i;
	/*argument char can be escaped and must be converted*/
	for(i=0; i < argc; i++){
	    char* current_arg = dest_buf+*index;
	    *index += unescape_string_copy_to_dest(args[i].val, args[i].vallen, dest_buf+*index);
	    dest_buf[ (*index)++ ] = '\0';
	    ZRT_LOG(L_BASE, "arg[%d] %s", i, current_arg );
	}
    }
    else{
	ZRT_LOG(L_BASE, "can't save arg %s, insufficient buffer size=%d/%d",
		source_buf, *index+source_buf_len+1, bufsize );
    }
}

void get_arg_array(char **args, char* buf, int bufsize){
    int idx=0;
    int handled_buf_idx=0;
    int i;
    while( idx < NVRAM_MAX_RECORDS_IN_SECTION ){
	for(i=handled_buf_idx; i < bufsize && idx < NVRAM_MAX_RECORDS_IN_SECTION; i++ ){
	    if ( buf[i] == '\0' ){
		args[idx++] = &buf[handled_buf_idx];
		handled_buf_idx = i+1;
		continue;
	    }
	}
	/*last item NULL pointer*/
	args[idx] = NULL;
	break;
    }
}

/*interface function
 while handling data it's saving it to buffer provided in obj1 and updating 
 used buffer space (index) in obj3 param*/
void handle_arg_record(struct MNvramObserver* observer,
		       struct ParsedRecord* record,
		       void* obj1, void* obj2, void* obj3){
    char* buffer = (char*)obj1; /*obj1 - char* */
    int bufsize = *(int*)obj2;  /*obj2 - int*  */
    int* index = (int*)obj3;    /*obj3 - int*  */
    assert(buffer); /*into buffer will be saved results*/
    assert(record);
    /*get param*/
    char* argval = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[ARG_PARAM_VALUE_KEY_INDEX], 
		      &argval);
    ZRT_LOG(L_SHORT, "arg: %s", argval);
    parse_transform_and_place_into_buf(buffer, bufsize, index,
				       argval, 
				       record->parsed_params_array[ARG_PARAM_VALUE_KEY_INDEX].vallen);
}

struct MNvramObserver* get_arg_observer(){
    struct MNvramObserver* self = &s_arg_observer;
    ZRT_LOG(L_INFO, "Create observer for section: %s", ARGS_SECTION_NAME);
    /*setup section name*/
    strncpy(self->observed_section_name, ARGS_SECTION_NAME, NVRAM_MAX_SECTION_NAME_LEN);
    /*setup section keys*/
    keys_construct(&self->keys);
    /*add keys and check returned key indexes that are the same as expected*/
    int key_index;
    /*check parameters*/
    key_index = self->keys.add_key(&self->keys, ARGS_PARAM_VALUE_KEY, NULL);
    assert(ARG_PARAM_VALUE_KEY_INDEX==key_index);

    /*setup functions*/
    s_arg_observer.handle_nvram_record = handle_arg_record;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", ARGS_SECTION_NAME);
    return &s_arg_observer;
}


