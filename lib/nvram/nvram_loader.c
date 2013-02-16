/*
 * nvram_loader.c
 *
 *  Created on: 01.12.2012
 *      Author: yaroslav
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "zrt_helper_macros.h"
#include "zrtlog.h"
#include "nvram_loader.h"
#include "conf_parser.h"

struct config_section_t{
    char* name;         /*section name in square brackets, it's allocated and should be freed*/
    int offset_start;   /*starting from header section*/
    int offset_end;     /*up to next header or till the end of file if last section*/
};

struct config_structure_t{
    struct config_section_t* sections;
    int count;
};

#define IS_VALID_POINTER_IN_RANGE(whole_data, whole_size, p) \
    (p != NULL && p >= whole_data && p < whole_data+whole_size )

#define IS_SQUARE_BRACKETS_ON_SAME_LINE(bound_start, bound_end, end_line) \
    ( bound_start != NULL && bound_end != NULL && (end_line == NULL || bound_end < end_line) )

/*Parse section name
 *@return allocated section struct if section name extracted correctly, 
 *NULL if no section name is located; section should be freed after use*/
static 
struct config_section_t* 
nearest_section_start(const char* whole_data, int whole_size, int* cursor_pos ){
    ZRT_LOG(L_INFO, "whole_size=%d, cursor_pos=%d", whole_size, *cursor_pos );
    struct config_section_t* new_section = NULL;    
    char* bound_start;
    do{
	bound_start = strchr(&whole_data[*cursor_pos], '[');
	if ( !IS_VALID_POINTER_IN_RANGE(whole_data,whole_size, bound_start) ){
	    ZRT_LOG(L_INFO, P_TEXT, "no begin of section located");
	    break;
	}
	char* bound_end = strchr(bound_start, ']');
	char* end_line = strchr(bound_start, '\n');
	/*check if section pointers are valid and we can continue retrieval of section*/
	if (IS_VALID_POINTER_IN_RANGE(whole_data,whole_size, &whole_data[*cursor_pos]) &&
	    IS_VALID_POINTER_IN_RANGE(whole_data,whole_size, bound_start) &&
	    IS_VALID_POINTER_IN_RANGE(whole_data,whole_size, bound_end) &&
	    IS_SQUARE_BRACKETS_ON_SAME_LINE(bound_start, bound_end, end_line) ) {
	    /*is it name of section located at begin of newline or in beginning of data*/
	    if ( bound_start == &whole_data[*cursor_pos] ||
		 ( bound_start > whole_data && bound_start[-1] == '\n' ) )
		{
		    /*alloc new section*/
		    new_section = calloc(1, sizeof(struct config_section_t*));
		    uint16_t striped_len;
		    const char* s = strip_all(bound_start+1, bound_end-bound_start-1, 
					      &striped_len );
		    new_section->name = calloc(striped_len+1, 1);
		    new_section->offset_start = bound_start - whole_data;
		    strncpy(new_section->name, s, striped_len );
		    ZRT_LOG(L_SHORT, "new section %s start =%d", 
			    new_section->name, new_section->offset_start );
		    //section located, set cursor_pos just after section name
		    *cursor_pos = bound_start - whole_data;
		}
	    else{
		//skip it, because it section name located not at the line begin
		*cursor_pos = bound_end - whole_data;
	    }
	}
	else{
	    //end of data
	    *cursor_pos = whole_size;
	}
    }while( new_section == NULL && *cursor_pos < whole_size );
    return new_section;
}

/*get complete list of configured sections*/
void get_config_structure(struct config_structure_t* conf_struct, struct NvramLoader* nvram){
    int cursor=0;
    /*get list of sections '[xxxx]' in configs*/
    memset(conf_struct, '\0', sizeof(struct config_structure_t) );
    struct config_section_t* new_section;

    /*get section names and starting pos for every section*/
    while( (new_section=nearest_section_start(nvram->nvram_data, nvram->nvram_data_size, 
					&cursor )) != NULL  ){
	ZRT_LOG(L_INFO, "section %s, cursor=%d", new_section->name, cursor);
	int curr_sect_index = conf_struct->count;
	++conf_struct->count;
	conf_struct->sections = realloc(conf_struct->sections, 
					conf_struct->count*sizeof(struct config_section_t));
	/*copy parsed section into array, and free memory*/
	memcpy(&conf_struct->sections[curr_sect_index], new_section,
	       sizeof(struct config_section_t));
	/*increment cursor to skip start of existing section */
	cursor += strlen(new_section->name); 
	ZRT_LOG(L_INFO, "section %s, newcursor=%d", new_section->name, cursor);
	free(new_section);
    }
    /*set end bounds for sections*/
    int i = conf_struct->count-1;
    int section_end = nvram->nvram_data_size;
    for ( ; i >= 0; i--){
	conf_struct->sections[i].offset_end = section_end;
	section_end = conf_struct->sections[i].offset_start;
    }
}


/*check section_name wanted to parse either valid or not
 *@param  section_name section name taken from nvram configuration file
 *@return -1 if not valid, or index of observer in observers array that intended to handle
 *specified section */
static int validate_section_name( struct NvramLoader* nvram, const char* section_name ){
    assert(section_name);
    int ret=0;
    /*match observer corresponding to section name*/
    struct MNvramObserver* matched_observer = NULL;
    int i;
    for (i=0; i < NVRAM_MAX_OBSERVERS_COUNT; i++ ){
	struct MNvramObserver* obs = nvram->nvram_observers[i];
	if ( obs != NULL && !strcasecmp( obs->observed_section_name, section_name ) ){
	    matched_observer = obs;
	    break;
	}
    }

    if ( matched_observer == NULL ){
	ZRT_LOG(L_ERROR, "Invalid nvram section %s", section_name);
	ret = -1;
    }
    else{
	
    }

    return ret;
}

/*@return parsde records count*/
static int parse_section( struct NvramLoader* nvram, const char* section_data, int count, 
			  struct MNvramObserver* observer ){
    assert(observer);
    assert(observer->keys);
    int parsed_records_count = 0;
    /*get records_array and records_count*/
    struct ParsedRecord* records_array = conf_parse( section_data, count, 
						    observer->keys,
						    &parsed_records_count);
    /*handle parsed data*/
    if ( records_array && parsed_records_count>0 ){
	/*parameters parsed correctly and seems to be correct*/
	ZRT_LOG(L_INFO, "nvram parsed record count=%d", parsed_records_count);
	int i;
	for(i=0; i < parsed_records_count; i++){
	    /*handle parsed record*/
	    observer->handle_nvram_record(observer, nvram, &records_array[i] );
	}
	/*free memories occupied by records*/
	free_records_array(records_array, parsed_records_count);
    }
    return parsed_records_count; 
}

void nvram_add_observer(struct NvramLoader* nvram, struct MNvramObserver* observer){
    assert(nvram);
    int i;
    for (i=0; i < NVRAM_MAX_OBSERVERS_COUNT; i++){
	if ( nvram->nvram_observers[i] == NULL ){
	    nvram->nvram_observers[i] = observer;
	    break;
	}
    }
    /*if folowing assert is raised then just increase defined max observers count*/
    assert(i<NVRAM_MAX_OBSERVERS_COUNT);
}

int nvram_read(struct NvramLoader* nvram, const char* nvram_file_name){
    ssize_t read_bytes=0;
    /*open nvram file and read a whole content in a single read operation*/
    int fd = open(nvram_file_name, O_RDONLY);
    if ( fd>0 ){
	read_bytes = read( fd, nvram->nvram_data, NVRAM_MAX_FILE_SIZE);
	close(fd);
    }
    nvram->nvram_data_size = read_bytes;
    return read_bytes;
}

int nvram_parse(struct NvramLoader* nvram){
    assert(nvram);
    int res = -1; /*error by default*/

    /*get initialized sections list*/
    struct config_structure_t conf_struct;
    get_config_structure(&conf_struct, nvram);
    ZRT_LOG(L_INFO, "sections count %d", conf_struct.count );
    
    /*get through list and parse sections*/
    int i;
    for( i=0; i < conf_struct.count; i++ ){
	const char* section_name = conf_struct.sections[i].name;
	int section_size = conf_struct.sections[i].offset_end 
	    - conf_struct.sections[i].offset_start;
	int validated_observer_index;
	if ( -1 != (validated_observer_index = 
		    validate_section_name(nvram, section_name )) ) {
	    ZRT_LOG(L_SHORT, "nvram successfully validated section %s", section_name );
	    res = parse_section(nvram, 
				&nvram->nvram_data[conf_struct.sections[i].offset_start],
				section_size, 
				nvram->nvram_observers[validated_observer_index]);
	    ZRT_LOG(L_SHORT, "nvram parsing result=%d for section: %s", res, section_name);
	}
	else{
	    ZRT_LOG(L_SHORT, "nvram invalid section %s", section_name);
	}
    }

    /*free config structure*/
    for(i=0; i < conf_struct.count; i++ )
	free(conf_struct.sections[i].name);
    free(conf_struct.sections);

    return res; 
}

void free_nvram_loader(struct NvramLoader* nvram){
    free(nvram);
}


struct NvramLoader* alloc_nvram_loader(
        struct MountsInterface* channels_mount,
        struct MountsInterface* transparent_mount ){
    /*alloc nvram struct*/
    struct NvramLoader* nvram = calloc(1, sizeof(struct NvramLoader));
    nvram->channels_mount = channels_mount;
    nvram->transparent_mount = transparent_mount;
    /*alloc array and fill cells by NULL, so unused cells would be stay NULL*/
    memset(nvram->nvram_observers, '\0', 
	   NVRAM_MAX_OBSERVERS_COUNT*sizeof(struct MNvramObserver*));
    nvram->read = nvram_read;
    nvram->add_observer = nvram_add_observer;
    nvram->parse = nvram_parse;
    nvram->free = free_nvram_loader;
    return nvram;
}


