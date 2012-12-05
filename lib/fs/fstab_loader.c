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

enum ParsingStatus {
        EStProcessing,
        EStToken,
        EStComment
};

struct parse_data{
    char* key;
    uint16_t keylen;
    char* val;
    uint16_t vallen;
};

#define CHANNEL_KEY "channel"
#define CHANNEL_INTERNAL 0x1

#define MOUNTPOINT_KEY "mountpoint"
#define MOUNTPOINT_INTERNAL 0x2

void log_record_error(int res){
    if ( CHECK_FLAG(res, CHANNEL_INTERNAL)  ){
	ZRT_LOG(L_ERROR, "'%s' parse error", CHANNEL_KEY);
    }
    if ( CHECK_FLAG(res, MOUNTPOINT_INTERNAL)  ){
	ZRT_LOG(L_ERROR, "'%s' parse error", MOUNTPOINT_KEY);
    }
}


const char* strip_all(const char* str, int len, uint16_t* striped_len ){
    int begin, end;
    /*strip from begin*/
    for( begin=0; begin < len; begin++ ){
        if ( str[begin] != ' ' && str[begin] != '\t' && str[begin] != '\n' ) break;
    }
    /*strip from end*/
    for( end=len-1; len >= 0; len-- ){
        if ( str[end] != ' ' && str[end] != '\t' && str[begin] != '\n' ) break;
    }
    *striped_len = end-begin+1;
    return MAX(0, str+begin);
}

/*return 0 if extracted, -1 if not*/
int extract_key_value( const char* src, int src_len,
        char** key, uint16_t* key_len, char** val, uint16_t*val_len ){
    /* just search '=' char, all before that will key, and rest will value*/
    char* delim = strchr(src, '=');
    if ( delim == NULL ) return -1; /*not valid pair*/
    int keylen = delim-src;
    /*keylen is full length, key_len, val_en are striped lengths*/
    *key = (char*)strip_all(src, keylen, key_len );
    *val = (char*)strip_all(delim+1, src_len-keylen-1, val_len );
    return 0;
}


int check_parsed_data_validity_handle_on_success(
						 struct FstabLoader* fstab_loader,
						 struct MFstabObserver* observer,
						 struct parse_data* array, int array_size){
    /*check that the type of parsed data is the same as expected*/
    char* channelname=NULL;
    char* mountpath=NULL;
    int res=0x3; /*flag bits that should be reseted to get check passed*/
    int i;
    for (i=0; i < array_size; i++){
        /*compare key1 ignoring case*/
        if ( !strncasecmp(CHANNEL_KEY, array[i].key, strlen(CHANNEL_KEY)) &&
	     array[i].keylen == strlen(CHANNEL_KEY) ){
            /*channel name fetched*/
            res &= ~CHANNEL_INTERNAL;
            free(channelname);
            channelname = calloc(array[i].vallen+1, 1);
            memcpy(channelname, array[i].val, array[i].vallen);
        }
        /*compare key2 ignoring case*/
        else if ( !strncasecmp(MOUNTPOINT_KEY, array[i].key, strlen(MOUNTPOINT_KEY)) &&
		  array[i].keylen == strlen(MOUNTPOINT_KEY) ){
            /*mountpath fetched*/
            res &= ~MOUNTPOINT_INTERNAL;
            free(mountpath);
            mountpath = calloc(array[i].vallen+1, 1);
            memcpy(mountpath, array[i].val, array[i].vallen);
        }
    }

    if ( !res ){
	assert(observer);
	ZRT_LOG(L_INFO, P_TEXT, "handle parsed record");
        /*parameters parsed correctly and seems to be correct*/
        res = observer->handle_fstab_record(fstab_loader, channelname, mountpath);
        /*free memories occupated by parameters*/
        free(channelname);
        free(mountpath);
        return res;
    }
    else{
        return res; //ERROR
    }
}


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
    int error = 0;
    int lex_cursor = -1;
    int lex_length = 0;
    /*items count is count of single record parameters*/
    struct parse_data parsed[RECORD_PARAMS_COUNT]; 
    int parsed_records_count=0;
    enum ParsingStatus st = EStProcessing;
    enum ParsingStatus st_prev = st;

    ZRT_LOG(L_INFO, P_TEXT, "parsing");
    int cursor=0;
    do {
        /*set comment stat if comment start found*/
        if ( fstab->fstab_contents[cursor] == '#' ){
	    if ( st == EStProcessing ){
		st_prev = EStComment;
		st = EStToken;
	    }
	    else{
		st = EStComment;
	    }
	    ZRT_LOG(L_EXTRA, "cursor=%d EStComment", cursor);
        }
        /* If comma in non comment state OR 
	 * for new line OR 
	 * if previos state was complete token
         * then set processing state to catch new processing data by cursor position*/
        else if ( (st != EStComment && fstab->fstab_contents[cursor] == ',') ||
                   fstab->fstab_contents[cursor] == '\n' ){
	    if ( st == EStProcessing ){
		/*if now processing data then handle it*/
		ZRT_LOG(L_EXTRA, "cursor=%d EStToken", cursor);
		st_prev = EStProcessing;
		st = EStToken;
	    }
	    else{
		/*start processing of significant data*/
		ZRT_LOG(L_EXTRA, "cursor=%d EStProcessing", cursor);
		st = EStProcessing;
		/*save begin of unprocessed data to know start position of unhandled data*/
		lex_cursor = cursor+1;
		/*set lex_length -1. it's will be incremented in 
		 *switch EStProcessing and lex_length value become valid*/
		lex_length=-1; 
	    }
        }
        ++cursor;

        switch(st){
        case EStProcessing:
            ++lex_length;
            break;
        case EStComment:
            lex_cursor = -1;
            lex_length=0;
            break;
        case EStToken:{
	    ZRT_LOG(L_INFO, "swicth EStToken: cursor=%d, lex_length=%d", cursor, lex_length);
	    uint16_t striped_token_len=0;
	    const char* striped_token = 
		strip_all(&fstab->fstab_contents[lex_cursor], lex_length, &striped_token_len );
	    ZRT_LOG(L_INFO, "swicth EStToken: striped token len=%d", striped_token_len);
	    /*If token has data, try to extract key and value*/
	    if ( striped_token_len > 0 ){
		/*parse pair 'key=value', strip spaces */
		if ( !extract_key_value( striped_token, striped_token_len,
					 &parsed[parsed_records_count].key, 
					 &parsed[parsed_records_count].keylen,
					 &parsed[parsed_records_count].val, 
					 &parsed[parsed_records_count].vallen ) ){
                    /*one of record parameters was parsed*/
                    /*increase counter of parsed data*/
                    ++parsed_records_count;
                    /*If get waiting count of record parameters*/
                    if ( parsed_records_count == RECORD_PARAMS_COUNT ){
                        int res = check_parsed_data_validity_handle_on_success
			    ( fstab, observer, parsed, RECORD_PARAMS_COUNT );
			parsed_records_count=0;
			if (res){
			    /*log error of parsing record*/
			    log_record_error(res);
			}
                    }
                }
		else{
		    char* error_text = calloc(lex_length+1, 1);
		    memcpy(error_text, &fstab->fstab_contents[lex_cursor], lex_length);
		    ZRT_LOG(L_ERROR, "fstab record: '%s' parsing error", error_text);
		    free(error_text);
		    error =-1;
		}
            }
	    /*restore processing state*/
	    st = st_prev;
	    /*save begin of unprocessed data to know start position of unhandled data*/
	    lex_cursor = cursor;
	    lex_length=0;
            break;
	}
        default:
            break;
        }

    }while( cursor < fstab->fstab_size );
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
