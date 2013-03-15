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

/*use OUTPUT_HASH_KEYS option only for debugging purposes, output file contains 
 *values and hashes and their size even bigger than input data size */
/*#define OUTPUT_HASH_KEYS*/

struct MapReduceUserIf;

void InitInterface( struct MapReduceUserIf* mr_if );

#endif /* USER_IMPLEM_H_ */
