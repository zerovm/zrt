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

#define MAX_PARAMS_COUNT 10

/*list of waiting keys in parsing data.
 *parsing parameter is pair key=value.*/
struct KeyList{
    char*  key_list[MAX_PARAMS_COUNT];
    int    key_count;
};
/*just add key to list, no checks for duplicated items doing.*/
void add_key_to_list(struct KeyList* list, const char* key);
/*free memories allocated for keys,  it's not free struct memory*/
void free_keylist(struct KeyList*);

/*single parsed parameter, part of single record*/
struct ParsedParam{
    int key_index; /*index in array of struct KeyList.key_list*/
    char* val;
    uint16_t vallen;
};

/*single parsed record*/
struct ParsedRecord{
    struct ParsedParam* parsed_params_array;
    /*parsed parameters count is the same as keys count in struct Keylist.key_count*/
};

/*parsing text data and observe all parsed data
 *@param text text to parse
 *@param len  text length to parse
 *@param key_list list of waiting keys
 *@param parsed_records_count pointer to get parsed records count
 *@return array or parsed records, get records count via param parsed_records_count
 */
struct ParsedRecord* conf_parse(const char* text, int len, struct KeyList* key_list,
				int* parsed_records_count);

#endif //CONF_PARSER_H

