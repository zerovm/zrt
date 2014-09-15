/*
 *
 * Copyright (c) 2013, LiteStack, Inc.
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

#ifndef __ZRTLOGBASE_H__
#define __ZRTLOGBASE_H__

#ifdef __ZRT_HOST
# define DONOT_USE_DYNMEMORY_FOR_LOGBASE
#endif //__ZRT_HOST

#define MAX_TEMP_BUFFER_SIZE 100
#define MAX_ITEMS_COUNT ELogItemsCount

/*Log Items*/
typedef enum {ELogLength=0, 
	      ELogAddress, 
	      ELogSize, 
	      ELogTitle, 
	      ELogTime, 
	      ELogCount, 
	      ELogIndex,
	      ELogPath,
	      ELogItemsCount
} log_item;

#ifdef DONOT_USE_DYNMEMORY_FOR_LOGBASE
extern char s_log_items[MAX_ITEMS_COUNT][MAX_TEMP_BUFFER_SIZE];
#else
extern char *s_log_items[MAX_ITEMS_COUNT];
#endif //DONOT_USE_DYNMEMORY_FOR_LOGBASE
extern int   s_log_items_count;

#ifdef DONOT_USE_DYNMEMORY_FOR_LOGBASE
# define LOG_BASE_ENABLE			\
    char s_log_items[MAX_ITEMS_COUNT][MAX_TEMP_BUFFER_SIZE];	\
    int s_log_items_count = 0
#else
# define LOG_BASE_ENABLE			\
    char* s_log_items[MAX_ITEMS_COUNT];		\
    int s_log_items_count = 0
#endif //DONOT_USE_DYNMEMORY_FOR_LOGBASE

#define LOG_DEBUG(item_id, value, comment)				\
    if( __zrt_log_is_enabled() ){					\
	char *buf__123;							\
	int debug_handle = __zrt_log_debug_get_buf(&buf__123);		\
	int len;							\
	if( debug_handle > 0 ){						\
	    assert( s_log_items[item_id] );				\
	    len = snprintf(buf__123, LOG_BUFFER_SIZE, "%s:%d " #value" ", \
			   BASEFILE__, __LINE__);				\
	    __zrt_log_write(debug_handle, buf__123, len, 0);		\
	    len = snprintf(buf__123, LOG_BUFFER_SIZE,			\
			   s_log_items[item_id], comment, (void*)(value) ); \
	    __zrt_log_write(debug_handle, buf__123, len, 0);		\
	}								\
    }


#define LOG_STDERR(item_id, value, comment)				\
    if( __zrt_log_is_enabled() ){					\
	char *buf__123;							\
	__zrt_log_debug_get_buf(&buf__123);				\
	int debug_handle = 2; /*stderr*/				\
	int len;							\
	if( debug_handle > 0 ){						\
	    assert( s_log_items[item_id] );				\
	    len = snprintf(buf__123, LOG_BUFFER_SIZE, s_log_items[item_id], comment, (void*)(value) ); \
	    __zrt_log_write(debug_handle, buf__123, len, 0);		\
	}								\
    }


#ifdef DONOT_USE_DYNMEMORY_FOR_LOGBASE
# define ITEM(itemid, name, format)				\
    {								\
	assert(s_log_items_count == itemid);			\
	assert(s_log_items_count < MAX_ITEMS_COUNT);		\
	snprintf(s_log_items[itemid], MAX_TEMP_BUFFER_SIZE,	\
		 "%s "#name"=%s\n",				\
		 "%-30s", format );				\
    }								\
	++s_log_items_count

#else
# define ITEM(itemid, name, format)					\
    {									\
	assert(s_log_items_count == itemid);				\
	char buf[MAX_TEMP_BUFFER_SIZE];					\
	int  buf_length = snprintf(buf, MAX_TEMP_BUFFER_SIZE,		\
				   "%s "#name"=%s\n",			\
				   "%-30s", format );			\
	s_log_items[itemid] = memcpy( malloc(buf_length), buf, buf_length); \
    }									\
	++s_log_items_count
#endif //DONOT_USE_DYNMEMORY_FOR_LOGBASE


/*put items creator of ITEM macroses into zrtlog.c*/
#define ITEMS_CREATOR				\
    ITEM(ELogLength,     length,     "%d" );	\
    ITEM(ELogAddress,    address,    "%p" );	\
    ITEM(ELogSize,       size,       "%u" );	\
    ITEM(ELogTitle,      =======,    "%s" );	\
    ITEM(ELogTime,       time,       "%s" );	\
    ITEM(ELogCount,      count,      "%d" );	\
    ITEM(ELogIndex,      index,      "%d" );	\
    ITEM(ELogPath,       path,       "%s" );


#endif //__ZRTLOGBASE_H__
