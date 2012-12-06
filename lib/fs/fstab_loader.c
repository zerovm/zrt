/*
 * fstab_loader.c
 *
 *  Created on: 01.12.2012
 *      Author: yaroslav
 */

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
#include "fstab_loader.h"
#include "conf_parser.h"


int fstab_read(struct FstabLoader* fstab, const char* fstab_file_name){
    /*open fstab file and read a whole content in a single read operation*/
    int fd = open(fstab_file_name, O_RDONLY);
    ssize_t read_bytes = read( fd, fstab->fstab_contents, FSTAB_MAX_FILE_SIZE);
    close(fd);
    if ( read_bytes > 0 ){
        fstab->fstab_size = read_bytes;
    }
    return read_bytes;
}

int fstab_parse(struct FstabLoader* fstab, struct MFstabObserver* observer){
    assert(fstab);
    assert(observer);
    int error = -1; /*error by default*/

    /*create key list*/
    struct KeyList key_list[RECORD_PARAMS_COUNT];
    memset(key_list, '\0', sizeof(struct KeyList)*RECORD_PARAMS_COUNT);
    
    add_key_to_list( key_list, PARAM_CHANNEL_KEY);
    add_key_to_list( key_list, PARAM_MOUNTPOINT_KEY);

    /*parse fstab file*/
    int records_count = 0;
    struct ParsedRecord* records_array = conf_parse(
						    fstab->fstab_contents, 
						    fstab->fstab_size, 
						    key_list,
						    &records_count);
    /*handle parsed data*/
    if ( records_array ){
	/*parameters parsed correctly and seems to be correct*/
	ZRT_LOG(L_INFO, "fstab parsed record count=%d", records_count);
	int i;
	for(i=0; i < records_count; i++){
	    /*access param value directly by hardcoded offset*/
	    char* channelname = calloc( records_array[i].parsed_params_array[0].vallen+1, 1 );
	    memcpy(channelname, 
		   records_array[i].parsed_params_array[0].val,
		   records_array[i].parsed_params_array[0].vallen);

	    char* mountpath = calloc( records_array[i].parsed_params_array[1].vallen+1, 1 );
	    memcpy(mountpath, 
		   records_array[i].parsed_params_array[1].val,
		   records_array[i].parsed_params_array[1].vallen);
	    /*handle parsed record*/
	    observer->handle_fstab_record(fstab, 
					  channelname, 
					  mountpath);
	    ZRT_LOG(L_SHORT, P_TEXT, "1");
	    /*free memories occupated by parameters*/
	    free(channelname);
	    free(mountpath);
	    ZRT_LOG(L_SHORT, P_TEXT, "2");
	    /*free handled record memory*/
	    free(records_array[i].parsed_params_array);
	    ZRT_LOG(L_SHORT, P_TEXT, "3");
	}
	ZRT_LOG(L_SHORT, P_TEXT, "5");
	/*free records array*/
	free(records_array);
	error = 0; /*OK*/
	ZRT_LOG(L_SHORT, P_TEXT, "6");
    }

    ZRT_LOG(L_SHORT, P_TEXT, "7");
    /*free keys*/
    free_keylist(key_list);
    ZRT_LOG(L_SHORT, P_TEXT, "8");
    return error; 
}


struct FstabLoader* alloc_fstab_loader(
        struct MountsInterface* channels_mount,
        struct MountsInterface* transparent_mount ){
    /*alloc fstab struct*/
    struct FstabLoader* fstab = calloc(1, sizeof(struct FstabLoader));
    fstab->channels_mount = channels_mount;
    fstab->transparent_mount = transparent_mount;
    fstab->read = fstab_read;
    fstab->parse = fstab_parse;
    return fstab;
}


void free_fstab_loader(struct FstabLoader* fstab){
    free(fstab);
}
