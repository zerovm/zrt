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


#define IS_VALID_POINTER_IN_RANGE(whole_data, whole_size, p) \
    (p != NULL && p >= whole_data && p < whole_data+whole_size )

#define IS_SQUARE_BRACKETS_ON_SAME_LINE(bound_start, bound_end, end_line) \
    ( bound_start != NULL && bound_end != NULL && (end_line == NULL || bound_end < end_line) )


/*section name, start, end*/
struct config_section_t{
    const char *name; /*section name, referred in config as name in square brackets*/
    int name_len;
    int offset_start;   /*starting from header section*/
    int offset_end;     /*up to next header or till the end of file if last section*/
};

/*list of sections*/
struct config_structure_t{
    struct config_section_t sections[NVRAM_MAX_SECTIONS_COUNT];
    int count;
};


/*Parse section name
 *@return filled section struct if section name extracted correctly, 
 *NULL if no section name is located; section should be freed after use*/
static 
struct config_section_t* 
get_nearest_section_start(struct config_section_t* section,
			  const char* whole_data, int whole_size, int* cursor_pos ){
    ZRT_LOG(L_EXTRA, "whole_size=%d, cursor_pos=%d", whole_size, *cursor_pos );
    struct config_section_t* new_section = NULL;    
    char* bound_start;
    do{
	bound_start = strchr(&whole_data[*cursor_pos], '[');
	if ( !IS_VALID_POINTER_IN_RANGE(whole_data,whole_size, bound_start) ){
	    ZRT_LOG(L_EXTRA, "valid section not located at pos:%d", *cursor_pos);
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
		    /*fill new section*/
		    new_section = section;
		    uint16_t striped_len;
		    const char* s = strip_all(bound_start+1, bound_end-bound_start-1, 
					      &striped_len );
		    new_section->name = s;
		    new_section->name_len = striped_len;
		    new_section->offset_start = bound_start - whole_data;
		    ZRT_LOG(L_EXTRA, "new section %s start =%d", 
			    GET_STRING(s,striped_len), new_section->offset_start );
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
void get_config_structure(struct NvramLoader* nvram, 
			  struct config_structure_t* sections ){
    int i;
    int cursor=0;
    sections->count=0;
    struct config_section_t* section = &sections->sections[sections->count];

    /*get section names and starting pos for every section*/
    while( NULL != get_nearest_section_start( section,
					     nvram->nvram_data, 
					     nvram->nvram_data_size, 
					     &cursor ) ){
	/*increment cursor to skip start of existing section */
	cursor += section->name_len;
	ZRT_LOG(L_INFO, "section %s, section data pos=%d", 
		GET_STRING(section->name, section->name_len), cursor);
	assert( sections->count < NVRAM_MAX_SECTIONS_COUNT );
	section = &sections->sections[++sections->count];
    }
    /*set end bounds for sections*/
    int section_end = nvram->nvram_data_size;
    for ( i = sections->count-1; i >= 0; i--){
	sections->sections[i].offset_end = section_end;
	section_end = sections->sections[i].offset_start;
    }
}


/*check section_name wanted to parse either valid or not and return valid obserber
 *@param  section_name section name taken from nvram configuration file
 *@param namelen section name length, because it's not null terminated
 *@return NULL if not valid, or observer that intended to handle specified section */
static struct MNvramObserver*
section_observer( struct NvramLoader* nvram, const char* section_name, int namelen ){
    assert(section_name);
    /*match observer corresponding to section name*/
    struct MNvramObserver* matched_observer = NULL;
    int i;
    for (i=0; i < NVRAM_MAX_OBSERVERS_COUNT; i++ ){
	struct MNvramObserver* obs = nvram->nvram_observers[i];
	if ( obs != NULL && 
	     !strncmp( obs->observed_section_name, section_name, namelen ) ){
	    matched_observer = obs;
	    break;
	}
    }

    if ( matched_observer == NULL ){
	ZRT_LOG(L_ERROR, 
		"Not found observer for nvram section %s", GET_STRING(section_name,namelen));
    }
    return matched_observer;
}

/*@param records pointer to future result
 *@return records*/
static struct ParsedRecords* 
parse_section( struct NvramLoader* nvram, 
	       struct ParsedRecords* records,
	       const char* section_data, int count, 
	       struct MNvramObserver* observer ){
    assert(nvram);
    assert(observer);
    assert(records);
    assert(section_data);
    /*parse records and handle parsed data*/
    if ( get_parsed_records(records, section_data, count, &observer->keys) ){
	/*parameters parsed correctly and seems to be correct*/
	records->observer = observer;
	/*print section detailed records*/
	ZRT_LOG(L_INFO, "nvram section [%s] has observer,records count=%d", 
		observer->observed_section_name, records->count);
	int i;
	struct ParsedRecord *r;
	for(i=0; i < records->count; i++){
	    if ( (r=&records->records[i]) != NULL ){
		ZRT_LOG(L_INFO, "nvram record #%d", i);
		int j=0;
		struct ParsedParam* p;
		while( j < observer->keys.count && 
		       (p=&r->parsed_params_array[j++]) != NULL ){
		    ZRT_LOG(L_EXTRA, "%s=%s", 
			    observer->keys.keys[p->key_index],
			    GET_STRING(p->val, p->vallen) );
		}
	    }
	}
    }
    return records->count?records:NULL; 
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
    /*open nvram file and read a whole content in a single read operation*/
    int fd = open(nvram_file_name, O_RDONLY);
    if ( fd>0 ){
	nvram->nvram_data_size = read( fd, nvram->nvram_data, NVRAM_MAX_FILE_SIZE);
	close(fd);
	ZRT_LOG(L_BASE, "nvram file size=%d", nvram->nvram_data_size);
    }
    return nvram->nvram_data_size;
}

void nvram_parse(struct NvramLoader* nvram){
    assert(nvram);

    /*get sections list and save it in nvram object*/
    struct config_structure_t sections_bounds;
    get_config_structure(nvram, &sections_bounds);
    ZRT_LOG(L_EXTRA, "nvram sections count %d", sections_bounds.count );

    /*go through list and parse sections*/
    struct MNvramObserver* observer;
    struct config_section_t* section; 
    int i;
    for( i=0; i < sections_bounds.count; i++ ){
	section = &sections_bounds.sections[i];
	/*if section is valid*/
	if ( (observer=section_observer(nvram, section->name, section->name_len )) ){
	    ZRT_LOG(L_EXTRA, "parse section %s", GET_STRING(section->name, section->name_len));
	    if ( parse_section(nvram, 
			       &nvram->parsed_sections[nvram->parsed_sections_count],
			       &nvram->nvram_data[section->offset_start],
			       section->offset_end - section->offset_start, 
			       observer) != NULL ){
		++nvram->parsed_sections_count;
	    }
	}
    }
}

void nvram_handle(struct NvramLoader* nvram, struct MNvramObserver* observer,
		  void* obj1, void* obj2, void* obj3){
    struct ParsedRecords* records;
    int i, j;
    for ( j=0; j < nvram->parsed_sections_count; j++ ){
	records = &nvram->parsed_sections[j];
	/*handle only records with observer matched */
	if ( observer==records->observer ){
	    for(i=0; i < records->count; i++){
		/*handle parsed record*/
		records->observer->handle_nvram_record(records->observer,  
						       &records->records[i],
						       obj1, obj2, obj3);
	    }
	}
    }
}

/*@return records count in section*/
struct ParsedRecords* nvram_section_by_name( struct NvramLoader* nvram, 
					     const char* section_name){
    /*Go through parsed sections and locate section by name*/
    int i;
    for(i=0; i < nvram->parsed_sections_count; i++ ){
	struct ParsedRecords* section = &nvram->parsed_sections[i];
	/*if section matched ignoring case*/
	if( !strcmp( section->observer->observed_section_name, section_name) )
	    return section;
    }
    return NULL;
}

    
struct NvramLoader* construct_nvram_loader(struct NvramLoader* nvram ){
    nvram->parsed_sections_count=0;
    /*fill cells by NULL, so unused cells would be stay NULL*/
    memset(nvram->nvram_observers, '\0', 
	   NVRAM_MAX_OBSERVERS_COUNT*sizeof(struct MNvramObserver*));
    nvram->read = nvram_read;
    nvram->add_observer = nvram_add_observer;
    nvram->parse = nvram_parse;
    nvram->handle = nvram_handle;
    nvram->section_by_name = nvram_section_by_name;
    return nvram;
}


