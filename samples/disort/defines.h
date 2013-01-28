/*
 * defines.h
 *
 *  Created on: 08.05.2012
 *      Author: YaroslavLitvinov
 */

#ifndef DEFINES_H_
#define DEFINES_H_

/*log section start*/
#define LOG_ERR 0
#define LOG_UI 1
#define LOG_DETAILED_UI 2
#define LOG_DEBUG 3
#define LOG_LEVEL LOG_UI

#define WRITE_FMT_LOG(level, fmt, args...) \
		if (level<=LOG_LEVEL ){\
			fprintf(stderr, fmt, args); \
		}
#define WRITE_LOG(level, str) \
		if (level<=LOG_LEVEL ){\
			fprintf(stderr, "%s\n", str); \
		}
/*log section end*/

enum { ESourceNode=1, EDestinationNode=2, EManagerNode=3, EInputOutputNode=4 };
#define ENV_SOURCE_NODE_NAME "SOURCE_NAME"
#define ENV_DEST_NODE_NAME   "DEST_NAME"
#define ENV_MAN_NODE_NAME    "MAN_NAME"

//9999872
/*array items count is passed to zmain as argv[1] and needed to set 
 *by set_items_count_to_sortjob*/
#define ARRAY_ITEMS_COUNT items_count_to_sortjob()
#define CHUNK_COUNT 1000
#define BASE_HISTOGRAM_STEP (ARRAY_ITEMS_COUNT/CHUNK_COUNT)
/*If MERGE_ON_FLY defined then sorted chunks received by destination nodes will only merged when obtained*/
#define MERGE_ON_FLY

////////////////////////// debug channels indetermenism
//#define ZEROVM_DEBUG
////////////////////////// debug


#endif /* DEFINES_H_ */
