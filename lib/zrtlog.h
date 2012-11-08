/*
 * zrtlog.h
 *
 *  Created on: 18.09.2012
 *      Author: yaroslav
 */

#ifndef ZRTLOG_H_
#define ZRTLOG_H_

#include "stdint.h"

/*log levels*/
#define L_SHORT 1
#define L_ERROR 1
#define L_INFO 2
#define L_EXTRA 3

#define VERBOSITY_ENV "VERBOSITY"

#ifndef DEBUG
#define DEBUG
#endif

/* Verbosity value that will filter log messages with less logging level
 * VERBOSITY environment variable should be set to increase verbosity
 * level, by default it will be set into 1, that value according to L_SHORT
 * incorrect values or 0 will be ignored, to switch logs off just remove
 * debugging channel "/dev/debug" from manifest*/

#ifdef DEBUG
#define LOG_BUFFER_SIZE 0x1000
#define ZRT_LOG(v_123, fmt_123, ...){					\
	int debug_handle_123;						\
	char *buf__123;							\
	if( verbosity() >= v_123 &&					\
	    (debug_handle_123=debug_handle_get_buf(&buf__123)) >= 0 ){	\
	    int len_123 = snprintf(buf__123, LOG_BUFFER_SIZE,		\
				   "%s; [%s]; %s, %d: " fmt_123 "\n",	\
				   __FILE__, syscall_stack_str(),	\
			   __func__, __LINE__, __VA_ARGS__);		\
	    zrtlog_write(debug_handle_123, buf__123, len_123, 0);	\
	}								\
    }


#define P_TEXT "%s"
#define P_PTR  "%p"
#define P_HEX  "0x%X"
#define P_INT  "%d"
#define P_UINT  "%u"
#define P_LONGINT  "%lld"
#define ZRT_LOG_PARAM(v_123, fmttype_123, param_123){			\
	int debug_handle_123;						\
	char *buf__123;							\
	if( verbosity() >= v_123 &&					\
	    (debug_handle_123=debug_handle_get_buf(&buf__123)) >= 0 ){	\
	    int len_123 = snprintf(buf__123, LOG_BUFFER_SIZE,		\
				   "%s param: " #param_123 "=" fmttype_123 "\n", \
				   __func__, param_123);		\
	    zrtlog_write(debug_handle_123, buf__123, len_123, 0);		\
	}								\
    }


#define ZRT_LOG_ERRNO( errcode ) \
    ZRT_LOG( L_SHORT, "errno=%d, %s", errcode, strerror(errcode) );


#define ZRT_LOG_DELIMETER						\
    do {								\
	char *buf__123;							\
	int debug_handle = debug_handle_get_buf(&buf__123);		\
	int len;							\
	if( debug_handle < 0) break;					\
	len = snprintf(buf__123, LOG_BUFFER_SIZE, "%060d\n", 0 );	\
	zrtlog_write(debug_handle, buf__123, len, 0);			\
    } while(0)


const char* syscall_stack_str();
void log_push_name( const char* name );
void log_pop_name( const char* name );
void set_zrtlog_fd(int fd);
int verbosity();
int zrtlog_fd();
void enable_logging_current_syscall();
void disable_logging_current_syscall();
int debug_handle_get_buf(char **buf);
int32_t zrtlog_write( int handle, const char* buf, int32_t size, int64_t offset);

#endif

#endif /* ZRTLOG_H_ */
