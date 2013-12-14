/*
 * parse text buffers and extract values in key=value format
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
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
    /*strip left*/
    for( begin=0; begin < len; begin++ ){
	if ( ! IS_IT_CHAR_TO_STRIP( str[begin] ) ) break;
    }
    /*strip right*/
    for( end=len-1; end>0 && len > 0; end-- ){
	if ( ! IS_IT_CHAR_TO_STRIP( str[end] ) ) break;
    }
    *striped_len = end-begin+1;
    /*self testing*/
    if ( *striped_len > len ){
	ZRT_LOG(L_ERROR, "begin=%d, end=%d, striped_len=%d, fulllen=%d", 
		begin, end, (int)*striped_len, len);
	assert(*striped_len<=len);
    }
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
    enum ParsingStatus st_new = st;
    int new_record_flag=0;

#ifdef PARSER_DEBUG_LOG
    ZRT_LOG(L_INFO, P_TEXT, "parsing");
#endif
    records->count=0;
    int cursor=0;
    do {
        /*set comment state if comment begin located*/
        if ( text[cursor] == COMMENT_CHAR ){
	    if ( st == EStProcessing ){
		/*retrieve already processed data, if has, before comment begin*/
		st_new = EStComment;
		st = EStToken;
	    }
	    else{
		/*just a comment, nothing processed previously*/
		st = EStComment;
	    }
#ifdef PARSER_DEBUG_LOG
	    ZRT_LOG(L_EXTRA, "cursor=%d EStComment", cursor);
#endif
        }
        /* If comma in non comment state OR 
	 * for new line OR 
	 * double quotes " begins text block for processing
	 * if previous state was complete token 
         * then set processing state to catch new processing data by cursor position*/
        else if ( (st != EStComment && ( text[cursor] == ',' )) 
		  || text[cursor] == '\n' ){
	    if ( st == EStProcessing ){
		/*if now processing data then handle it*/
#ifdef PARSER_DEBUG_LOG
		ZRT_LOG(L_EXTRA, "cursor=%d EStToken", cursor);
#endif
		st = EStToken;
		if ( text[cursor] == '\n' )
		    new_record_flag = 1;
		else
		    st_new = EStProcessing;
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
	/*right bound reached of processing data*/
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
	    /*token now is ready to retrieve and check*/
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
			ZRT_LOG(L_INFO, "key (len=%d,index=%d) '%s' found, keyval=%s,", 
				key_val_parse.keylen, parsed_key_index,
				GET_STRING(key_val_parse.key,
					   key_val_parse.keylen),
				GET_STRING(key_val_parse.val,
					   key_val_parse.vallen));
#endif
			if ( parsed_key_index >= 0 ){
			    if ( temp_keys_parsed[parsed_key_index].key != NULL ){
				/*parsed item with the same key already saved, and new 
				  one will be ignored*/
#ifdef PARSER_DEBUG_LOG
				ZRT_LOG(L_ERROR, P_TEXT, "last key duplicated, ");
#endif
			    }
			    else{
				ZRT_LOG(L_INFO, "parsed key(len=%d)=%s saved", 
					key_val_parse.vallen,
					GET_STRING(key_val_parse.val, key_val_parse.vallen) );

				/*save parsed key,value*/
				temp_keys_parsed[parsed_key_index] = key_val_parse;
				/*one of record parameters was parsed*/
				/*increase counter of parsed data*/
				++parsed_params_count;
			    }
			}
			else{
			    /*wrong key*/
			    ZRT_LOG(L_ERROR, "wrong key, key=%s", 
				GET_STRING(key_val_parse.key, key_val_parse.keylen));
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
	    if ( new_record_flag ){
		new_record_flag=0;
		/*free previosly parsed results, new record start*/
		parsed_params_count=0;
		memset(&temp_keys_parsed, '\0', sizeof(temp_keys_parsed) );
	    }
	    /*restore processing state*/
	    st = st_new;
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


/*****************************************
 * command line arguments parsing
 *****************************************/

/*parsed arg, save it*/
#define SAVE_PARSED_ARG(records, r_array_len, r_count, parsed)	\
    if ( r_count < r_array_len ){				\
	records[(r_count)++] = parsed;				\
	(parsed).val = NULL, (parsed).vallen=0;			\
    }
    

int parse_args(struct ParsedParam* parsed_args_array, int args_array_len,
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

#define MATCH_HEX(chr)							\
    ((chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'z') ||	\
     (chr >= 'A' && chr <= 'Z'))? 1 : 0
/*example: aa, 0a*/
#define MATCH_2HEX_DIGITS(hexstr)				\
    (MATCH_HEX( (hexstr)[0] ) != 0 && MATCH_HEX( (hexstr)[1] ) != 0)
#define MATCH_2ESCAPING_CHARS(stre)			\
    ((stre)[0] == '\\' && (stre)[1] == 'x')? 1 : 0

int unescape_string_copy_to_dest(const char* source, int sourcelen, char* dest){
    char tmp[3];
    int index_in=0;
    int index_out=0;
    while( index_in < sourcelen ){
	/*try to escape if has enough length for conversion*/
	if ( index_in + 3 < sourcelen /*if escape sequence has enough len*/ && 
	     MATCH_2ESCAPING_CHARS(source+index_in) &&
	     MATCH_2HEX_DIGITS(source+index_in+2) ){
	    /*escaping chars and hex values are validated*/
	    char* hex = (char*)(source+index_in+2);
	    tmp[0] = hex[0];
	    tmp[1] = hex[1];
	    tmp[2] = '\0';
	    dest[index_out++] = str_hex_to_int_not_using_locale(tmp);
	    index_in+=4;
	}
	else{
	    dest[index_out++] = source[index_in++];
	}
    }
    return index_out;
 }

/*convert validated single hex character into integer*/
#define GET_VALIDATED_HEX_CHAR_TO_INT(hexchar, hexint)				\
    if (hexchar >= '0' && hexchar <= '9')				\
	hexint = hexchar-'0';						\
    if (hexchar >= 'a' && hexchar <= 'z')				\
	hexint = 10+hexchar-'a';						\
    if (hexchar >= 'A' && hexchar <= 'Z')				\
	hexint = 10+hexchar-'A';


char str_hex_to_int_not_using_locale(char * two_digits_str)
{
    char temp;
    char res;
    GET_VALIDATED_HEX_CHAR_TO_INT(two_digits_str[0], temp);
    temp <<= 4;
    GET_VALIDATED_HEX_CHAR_TO_INT(two_digits_str[1], res);
    res |= temp;
    return res;
}

struct ParsedRecord* copy_record( const struct ParsedRecord* record, struct ParsedRecord* result_record ){
    int i;
    const struct ParsedParam* p_in;
    struct ParsedParam* p_out;
    for (i=0; i < NVRAM_MAX_KEYS_COUNT_IN_RECORD; i++){
	p_in  = &record->parsed_params_array[i];
	p_out = &result_record->parsed_params_array[i];
	if ( p_in->val ){
	    p_out->key_index = p_in->key_index;
	    p_out->vallen    = p_in->vallen;
	    p_out->val = malloc(p_in->vallen+1);
	    memcpy(p_out->val, p_in->val, p_in->vallen);
	    p_out->val[p_in->vallen] = '\0';
	}
    }
    return result_record;
}

void free_record_memories(struct ParsedRecord* record){
    int i;
    struct ParsedParam* p;
    for (i=0; i < NVRAM_MAX_KEYS_COUNT_IN_RECORD; i++){
	p  = &record->parsed_params_array[i];
	free(p->val);
	p->val = NULL;
    }
}

//end of file
