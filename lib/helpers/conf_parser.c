/* parse text buffers and extract values in key=value format
 * comments started with '#' are allowed
 * 
 * conf_parser.c
 *
 *  Created on: 5.12.2012
 *      Author: YaroslavLitvinov
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <alloca.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "conf_parser.h"
#include "conf_keys.h"

//#define PARSER_DEBUG_LOG

#define IS_IT_CHAR_TO_STRIP(chr) strchr(STRIPING_CHARS, chr)

enum ParsingStatus{
    EStComment,
    EStProcessing,
    EStToken
};

struct internal_parse_data{
    char* key;
    uint16_t keylen;
    char* val;
    uint16_t vallen;
};


/********************************************
 * parsing helpers
 ********************************************/
const char* strip_all(const char* str, int len, uint16_t* striped_len ){
    int begin, end;
    /*strip from begin*/
    for( begin=0; begin < len; begin++ ){
	if ( ! IS_IT_CHAR_TO_STRIP( str[begin] ) ) break;
    }
    /*strip from end*/
    for( end=len-1; len > 0; end-- ){
	if ( ! IS_IT_CHAR_TO_STRIP( str[end] ) ) break;
    }
    *striped_len = end-begin+1;
    return MAX(0, str+begin);
}

/*return 0 if extracted, -1 if not*/
int extract_key_value( const char* src, int src_len,
		       char** key, uint16_t* key_len, char** val, uint16_t*val_len ){
    /* just search KEY_VALUE_DELIMETER ('=') char, all before that will key, 
       and rest of data would be value*/
    char* delim = strchr(src, KEY_VALUE_DELIMETER );
    if ( delim == NULL || delim > src+src_len ) return -1; /*not valid pair*/
    int keylen = delim-src;
    /*keylen is full length, key_len, val_en are striped lengths*/
    *key = (char*)strip_all(src, keylen, key_len );
    *val = (char*)strip_all(delim+1, src_len-keylen-1, val_len );
    return 0;
}

/*****************************************
 * parse and save parsed data, if during parsing any unsupported key
 * is located then current record will be discarded and return NULL
 *****************************************/

static 
struct ParsedRecord* 
get_parsed_record(struct ParsedRecord* record,
		  const struct KeyList* keys,
		  struct internal_parse_data* params_array, 
		  int params_count){
    assert(keys);
    int i;
    for(i=0; i < params_count; i++ ){
	/*if key matched it return key index, in specified list of expecting keys so it's
	 *guarantied that key always has determinded index even for unspecified their order*/
	int key_index =  keys->find(keys, params_array[i].key, params_array[i].keylen);
	/*if current key is wrong*/
	if ( key_index < 0 ){
#ifdef PARSER_DEBUG_LOG
	    ZRT_LOG(L_ERROR, "invalid key '%s'", 
		    GET_STRING(params_array[i].key, params_array[i].keylen ) );
#endif
	    return NULL; /*error*/
	}
	else{
	    /*save current param*/
	    record->parsed_params_array[key_index].key_index = key_index; /*param index*/
	    record->parsed_params_array[key_index].val = params_array[i].val;
	    record->parsed_params_array[key_index].vallen = params_array[i].vallen;
	}
    }
    return record;
}


struct ParsedRecords* get_parsed_records(struct ParsedRecords* records,
					 const char* text, int len, struct KeyList* key_list){
    assert(records);
    assert(text);
    assert(key_list);
    int lex_cursor = 0;
    int lex_length = 0;
    int parsed_params_count=0;
    struct internal_parse_data key_val_parse;
    struct internal_parse_data temp_keys_parsed[NVRAM_MAX_KEYS_COUNT_IN_RECORD]; 
    memset(&temp_keys_parsed, '\0', sizeof(temp_keys_parsed));
    enum ParsingStatus st = EStProcessing;
    enum ParsingStatus st_prev = st;

#ifdef PARSER_DEBUG_LOG
    ZRT_LOG(L_INFO, P_TEXT, "parsing");
#endif
    records->count=0;
    int cursor=0;
    do {
        /*set comment stat if comment start found*/
        if ( text[cursor] == '#' ){
	    if ( st == EStProcessing ){
		st_prev = EStComment;
		st = EStToken;
	    }
	    else{
		st = EStComment;
	    }
#ifdef PARSER_DEBUG_LOG
	    ZRT_LOG(L_EXTRA, "cursor=%d EStComment", cursor);
#endif
        }
        /* If comma in non comment state OR 
	 * for new line OR 
	 * if previos state was complete token 
         * then set processing state to catch new processing data by cursor position*/
        else if ( (st != EStComment && text[cursor] == ',') ||
		  text[cursor] == '\n' ){
	    if ( st == EStProcessing ){
		/*if now processing data then handle it*/
#ifdef PARSER_DEBUG_LOG
		ZRT_LOG(L_EXTRA, "cursor=%d EStToken", cursor);
#endif
		st_prev = EStProcessing;
		st = EStToken;
	    }
	    else{
		/*start processing of significant data*/
#ifdef PARSER_DEBUG_LOG
		ZRT_LOG(L_EXTRA, "cursor=%d EStProcessing", cursor);
#endif
		st = EStProcessing;
		/*save begin of unprocessed data to know start position of unhandled data*/
		lex_cursor = cursor+1;
		/*set lex_length -1. it's will be incremented in 
		 *switch EStProcessing and lex_length value become valid*/
		lex_length=-1; 
	    }
        }
	/*parsing data right bound reached*/
	else if (cursor == len-1){
	    /*extend current token up to last char*/
	    ++lex_length;
	    st = EStToken;
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
#ifdef PARSER_DEBUG_LOG
	    ZRT_LOG(L_INFO, "swicth EStToken: lex_cursor=%d, lex_length=%d, pointer=%p", 
		    lex_cursor, lex_length, &text[lex_cursor]);
	    /*log non-striped lexema*/
	    ZRT_LOG(L_EXTRA, "lex= '%s'", GET_STRING(&text[lex_cursor], lex_length));
#endif
	    uint16_t striped_token_len=0;
	    const char* striped_token = 
		strip_all(&text[lex_cursor], lex_length, &striped_token_len );
#ifdef PARSER_DEBUG_LOG
	    ZRT_LOG(L_INFO, "swicth EStToken: striped token len=%d, pointer=%p", 
		    striped_token_len, striped_token);
#endif
	    /*If token has data, try to extract key and value*/
	    if ( striped_token_len > 0 ){
#ifdef PARSER_DEBUG_LOG
		/*log striped token*/
		ZRT_LOG(L_INFO, "token= '%s'", GET_STRING(striped_token, striped_token_len));
#endif
		/*parse pair 'key=value', strip spaces */
		int parsed_key_index = -1;
		if ( !extract_key_value( striped_token, striped_token_len,
					 &key_val_parse.key, 
					 &key_val_parse.keylen,
					 &key_val_parse.val, 
					 &key_val_parse.vallen ) 
		     )
		    {
			/*get key index*/
			parsed_key_index = key_list->find(key_list, 
							  key_val_parse.key,
							  key_val_parse.keylen);
#ifdef PARSER_DEBUG_LOG
			ZRT_LOG(L_INFO, "key found, key=%s, len=%d index=%d", 
				GET_STRING(key_val_parse.key,
					   key_val_parse.keylen),
				key_val_parse.keylen, parsed_key_index);
#endif
			if ( parsed_key_index >= 0 ){
			    if ( temp_keys_parsed[parsed_key_index].key != NULL ){
				/*parsed item with the same key already saved, and new 
				  one will be ignored*/
#ifdef PARSER_DEBUG_LOG
				ZRT_LOG(L_ERROR, P_TEXT, "last key duplicated, skipped");
#endif
			    }
			    else{
				/*save parsed key,value*/
				temp_keys_parsed[parsed_key_index] = key_val_parse;
				/*one of record parameters was parsed*/
				/*increase counter of parsed data*/
				++parsed_params_count;
			    }
			}
		    }
		else{
#ifdef PARSER_DEBUG_LOG
		    ZRT_LOG(L_ERROR, P_TEXT, "last token parsing error");
#endif
		}

		    
		/*If get waiting count of record parameters*/
		if ( key_list->count == parsed_params_count ){
#ifdef PARSER_DEBUG_LOG
		    ZRT_LOG(L_INFO, "key_list->count =%d", parsed_params_count);
#endif
		    /*parsed params count is enough to save it as single record.
		     *add it to parsed records array*/
		    struct ParsedRecord record;
		    if ( get_parsed_record(&record,
					   key_list, temp_keys_parsed, parsed_params_count) ){
			/*record parsed OK*/
#ifdef PARSER_DEBUG_LOG
			ZRT_LOG(L_INFO, "save record #%d OK", records->count);
#endif
			records->records[records->count++] = record;
		    }
		    /* current record parsed, reset params count 
		     * to be able parse new record*/
		    parsed_params_count=0;
		    memset(&temp_keys_parsed, '\0', sizeof(temp_keys_parsed) );
                }
            }
	    /*restore processing state*/
	    st = st_prev;
#ifdef PARSER_DEBUG_LOG
	    ZRT_LOG(L_INFO, P_TEXT, "restore previous parsing state");
#endif
	    /*save begin of unprocessed data to know start position of unhandled data*/
	    lex_cursor = cursor;
	    lex_length=0;
            break;
	}
        default:
            break;
        }

    }while( cursor < len );
#ifdef PARSER_DEBUG_LOG
    ZRT_LOG(L_INFO, P_TEXT, "Section parsed");
#endif
    return records; /*complete if OK, or NULL if error*/
}

//end of file
