/*
 * user_implem.h
 *
 *  Created on: 15.07.2012
 *      Author: yaroslav
 */

#ifndef USER_IMPLEM_H_
#define USER_IMPLEM_H_

#define ENV_MAP_NODE_NAME "MAP_NAME"
#define ENV_REDUCE_NODE_NAME "REDUCE_NAME"

#define STDIN  0
#define STDOUT 1 //fd

#define HASH_TYPE         uint32_t
#define HASH_SIZE         sizeof(uint32_t)
#define HASH_STR_LEN      HASH_SIZE*2+1
//#define HASH_TYPE         uint16_t
//#define HASH_SIZE         sizeof(uint16_t)
//#define HASH_STR_LEN      HASH_SIZE*2+1

#define IO_BUF_SIZE       0x100000

/* It's switching mapreduce library use cases for test purposes. 
 * Set VALUE_ADDR_AS_DATA into 1 to get high performance wordcount, 
 * or set into 0 to test key data with length upper than 4 bytes*/
#define VALUE_ADDR_AS_DATA     1
//#define VALUE_ADDR_AS_DATA     0

/*Default data value for new Map items*/
#define DEFAULT_INT_VALUE 1
#define DEFAULT_STR_VALUE "1"

struct MapReduceUserIf;

void InitInterface( struct MapReduceUserIf* mr_if );

#endif /* USER_IMPLEM_H_ */
