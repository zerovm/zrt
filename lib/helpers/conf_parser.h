/*
 * conf_parser.h
 * engine for parsing text, with comments started with '#' support.
 * Expected that parsing elemenets are divided by commas ',',
 * in format key=value.
 *
 *  Created on: 5.12.2012
 *      Author: YaroslavLitvinov
 */

#ifndef CONF_PARSER_H
#define CONF_PARSER_H

#include <stdint.h>

#define KEY_VALUE_DELIMETER '='
#define STRIPING_CHARS      " \t\n"

/*forward declarations*/
struct KeyList;

/*single parsed parameter, part of single record*/
struct ParsedParam{
    int key_index; /*index in array of struct KeyList.key_list*/
    char* val;
    uint16_t vallen;
};

#define ALLOC_PARAM_VALUE(parsed_param, str_value_pp){		\
	*str_value_pp = calloc( parsed_param.vallen+1, 1 );		\
	memcpy( *str_value_pp, parsed_param.val, parsed_param.vallen); \
    }


/*single parsed record*/
struct ParsedRecord{
    /*parsed parameters count is the same as keys count in struct Keylist.key_count*/
    struct ParsedParam* parsed_params_array;
};

/*parsing text data and observe all parsed data
 *@param parsed_records_count pointer to get records count
 *@param text text to parse
 *@param len  text length to parse
 *@param key_list list of waiting keys
 *@param parsed_records_count pointer to get parsed records count
 *@return array or parsed records, caller is responsible to free array
 * by using free_records_array
 */
struct ParsedRecord* conf_parse(const char* text, int len, struct KeyList* key_list,
				int* parsed_records_count);

void free_records_array(struct ParsedRecord *records, int count);

const char* strip_all(const char* str, int len, uint16_t* striped_len );

#endif //CONF_PARSER_H

