#ifndef __ZRTLOGBASE_H__
#define __ZRTLOGBASE_H__

#define MAX_TEMP_BUFFER_SIZE 100
#define MAX_ITEMS_SIZE 10

/*Log Items*/
typedef enum {ELogLength=0, ELogAddress, ELogSize, ELogTitle, ELogTime, ELogCount} log_item;

extern char* s_log_items[MAX_ITEMS_SIZE];
extern int   s_log_items_count;

#define LOG_BASE_ENABLE				\
    char* s_log_items[MAX_ITEMS_SIZE];		\
    int s_log_items_count = 0

#define LOG_DEBUG(item_id, value, comment)				\
    if( __zrt_log_is_enabled() ){					\
	char *buf__123;							\
	int debug_handle = __zrt_log_debug_get_buf(&buf__123);		\
	int len;							\
	if( debug_handle > 0 ){						\
	    assert( s_log_items[item_id] );				\
	    len = snprintf(buf__123, LOG_BUFFER_SIZE, s_log_items[item_id], comment, (void*)(value) ); \
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

#define ITEM(itemid, value, name, format){				\
	assert(s_log_items_count == itemid);				\
	char buf[MAX_TEMP_BUFFER_SIZE];					\
	int  buf_length = snprintf(buf, MAX_TEMP_BUFFER_SIZE,		\
				   " L_BASE %s:%d %s "#name"=%s\n",		\
				   __FILE__, __LINE__, "%-30s", format ); \
	s_log_items[itemid] = memcpy( malloc(buf_length), buf, buf_length); \
	++s_log_items_count;						\
    }

/*put items creator of ITEM macroses into zrtlog.c*/
#define ITEMS_CREATOR						\
    ITEM(ELogLength,     value, length,     "%d" )		\
    ITEM(ELogAddress,    value, address,    "0x%x" )		\
    ITEM(ELogSize,       value, size,       "%d" )		\
    ITEM(ELogTitle,      value, =======,    "%s" )		\
    ITEM(ELogTime,       value, time,       "%s" )		\
    ITEM(ELogCount,      value, count,      "%d" )



#endif //__ZRTLOGBASE_H__
