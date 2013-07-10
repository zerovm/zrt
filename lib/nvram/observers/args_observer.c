/*
 * args_observer.c
 *
 *  Created on: 3.06.2013
 *      Author: yaroslav
 */

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

static void add_val_to_temp_buffer(char* buf, int bufsize, int* index,
				    const char* val, int len){
    /*add arg pairs into buffer, every pair end must be null term char '\0' */
    if ( *index+len < bufsize ){
	/*all arguments coming unparsed, parse it here*/
	struct ParsedParam args[NVRAM_MAX_RECORDS_IN_SECTION];
	/*in case if too many args will be parsed they will be skipped*/
	int argc = parse_args(args, NVRAM_MAX_RECORDS_IN_SECTION, val, len);
	ZRT_LOG(L_BASE, "argc= %d", argc );
	int i;
	/*argument char can be escaped and must be converted*/
	for(i=0; i < argc; i++){
	    char* current_arg = buf+*index;
	    *index += unescape_string_copy_to_dest(args[i].val, args[i].vallen, buf+*index);
	    buf[ (*index)++ ] = '\0';
	    ZRT_LOG(L_BASE, "arg[%d] %s", i, current_arg );
	}
    }
    else{
	ZRT_LOG(L_BASE, "can't save arg %s, insufficient buffer size=%d/%d",
		val, *index+len+1, bufsize );
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
    add_val_to_temp_buffer(buffer, bufsize, index,
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
    key_index = self->keys.add_key(&self->keys, ARGS_PARAM_VALUE_KEY);
    assert(ARG_PARAM_VALUE_KEY_INDEX==key_index);

    /*setup functions*/
    s_arg_observer.handle_nvram_record = handle_arg_record;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", ARGS_SECTION_NAME);
    return &s_arg_observer;
}


