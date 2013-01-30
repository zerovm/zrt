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

//formating types 
#define P_TEXT "%s"
#define P_PTR  "%p"
#define P_HEX  "0x%X"
#define P_INT  "%d"
#define P_UINT  "%u"
#define P_LONGINT  "%lld"


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

/*ZRT_LOG
 v_123 verbosity param, fmt_123 format string, ... arguments*/
#define ZRT_LOG(v_123, fmt_123, ...){					\
	int debug_handle_123;						\
	char *buf__123;							\
	if( verbosity() >= v_123 &&					\
	    (debug_handle_123=debug_handle_get_buf(&buf__123)) >= 0 ){	\
	    int len_123 = snprintf(buf__123, LOG_BUFFER_SIZE,		\
				   #v_123 " %s; [%s]; %s, %d: " fmt_123 "\n", \
				   __FILE__, syscall_stack_str(),	\
			   __func__, __LINE__, __VA_ARGS__);		\
	    zrtlog_write(debug_handle_123, buf__123, len_123, 0);	\
	}								\
    }


#define ZRT_LOG_PARAM(v_123, fmttype_123, param_123){	\
	ZRT_LOG(v_123, fmttype_123, param_123);		\
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

/* ******************************************************************************
 * Syscallbacks debug macros*/

/* Push current NACL syscall into logging stack that printing for every log invocation.
 * Enable logging for NACL syscall, and printing arguments*/
#define LOG_SYSCALL_START(fmt_123, ...) {				\
	log_push_name(__func__);					\
	enable_logging_current_syscall();				\
	ZRT_LOG(L_INFO, fmt_123, __VA_ARGS__);				\
    }

/* Pop from logging stack current syscall function.
 * Prints retcode and errno*/
#define LOG_SYSCALL_FINISH(ret, fmt_123, ...){				\
        if ( ret < 0 ){							\
	    ZRT_LOG(L_SHORT, "ret=0x%x errno=%d " fmt_123 "",		\
		    (int)ret, errno, __VA_ARGS__);			\
	}								\
        else{								\
	    ZRT_LOG(L_SHORT, "ret=0x%x " fmt_123 "",			\
		    (int)ret, __VA_ARGS__);				\
	}								\
        log_pop_name(__func__);						\
    }

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

#else
#define LOG_SYSCALL_START(fmt_123, ...)
#define LOG_SYSCALL_FINISH()

#endif

#endif /* ZRTLOG_H_ */
