/*
 * zrtlog.h
 *
 *  Created on: 18.09.2012
 *      Author: yaroslav
 */

#ifndef ZRTLOG_H_
#define ZRTLOG_H_

#include <stdint.h>
#include <stdio.h> //snprintf
#include "printf_prolog.h" //tfp_sprintf
#include "channels_reserved.h"
#include "zrtlogbase.h"

/*log levels*/
#define L_BASE   1   /*log only base zrt data*/
#define L_ERROR  1   /*log errors*/
#define L_SHORT  2  
#define L_INFO   3
#define L_EXTRA  4

/*verbosity not available during prolog stage before nvram file parsed,
  so use default verbosity value defined here, especially for that*/
#define DEFAULT_VERBOSITY_FOR_PROLOG_LOG L_SHORT

//formating types 
#define P_TEXT "%s"
#define P_PTR  "%p"
#define P_HEX  "0x%X"
#define P_INT  "%d"
#define P_UINT  "%u"
#define P_LONGINT  "%lld"

#define LOG_FUNCTION_STACK_NAMES
//#undef  LOG_FUNCTION_STACK_NAMES

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
#define ZRT_LOG(v_123, fmt_123, ...)					\
    if ( __zrt_log_is_enabled() && __zrt_log_fd() > 0 ){		\
	if ( __zrt_log_prolog_mode_is_enabled() ){			\
	    if ( DEFAULT_VERBOSITY_FOR_PROLOG_LOG >= v_123 ){		\
		/*write directly into channel always if logfile defined*/ \
		tfp_printf("PROLOG " #v_123 " %s:%d " fmt_123 "\n",	\
			   __FILE__, __LINE__, __VA_ARGS__);		\
		/*flush data prepared in internal buffer by tfp_printf*/ \
		__zrt_log_write(__zrt_log_fd(), NULL, 0, 0);		\
	    }								\
	}								\
	else{								\
	    int debug_handle_123;					\
	    char *buf__123;						\
	    if( __zrt_log_verbosity() >= v_123 &&			\
		(debug_handle_123=__zrt_log_debug_get_buf(&buf__123)) >= 0 ){ \
		int len_123 = snprintf(buf__123, LOG_BUFFER_SIZE,	\
				       #v_123 " %s:%d; [%s]- " fmt_123 "\n", \
				       __FILE__, __LINE__,		\
				       __zrt_log_syscall_stack_str(), __VA_ARGS__); \
		__zrt_log_write(debug_handle_123, buf__123, len_123, 0); \
	    }								\
	}								\
    }									\


#define ZRT_LOG_DELIMETER  if( __zrt_log_is_enabled() ){ 		\
	char *buf__123;							\
	int debug_handle = __zrt_log_debug_get_buf(&buf__123);		\
	int len;							\
	if( debug_handle > 0 && __zrt_log_verbosity() >= L_SHORT ){	\
	    len = snprintf(buf__123, LOG_BUFFER_SIZE, "%060d\n", 0 );	\
	    __zrt_log_write(debug_handle, buf__123, len, 0);		\
	}								\
    }

/* ******************************************************************************
 * Syscallbacks debug macros*/

/* Push current NACL syscall into logging stack that printing for every log invocation.
 * Enable logging for NACL syscall, and printing arguments*/
#define LOG_SYSCALL_START(fmt_123, ...) {	\
	__zrt_log_push_name(__func__);		\
	ZRT_LOG(L_INFO, fmt_123, __VA_ARGS__);	\
    }

#define LOG_SHORT_SYSCALL_FINISH(ret, fmt_123, ...){		\
        if ( ret < 0 ){						\
	    ZRT_LOG(L_SHORT, "ret=0x%x errno=%d " fmt_123 "",	\
		    (int)ret, errno, __VA_ARGS__);		\
	}							\
        else{							\
	    ZRT_LOG(L_SHORT, "ret=0x%x " fmt_123 "",		\
		    (int)ret, __VA_ARGS__);			\
	}							\
        __zrt_log_pop_name(__func__);				\
    }


/* Pop from logging stack current syscall function.
 * Prints retcode and errno*/
#define LOG_INFO_SYSCALL_FINISH(ret, fmt_123, ...){		\
        if ( ret < 0 ){						\
	    ZRT_LOG(L_INFO, "ret=0x%x errno=%d " fmt_123 "",	\
		    (int)ret, errno, __VA_ARGS__);		\
	}							\
        else{							\
	    ZRT_LOG(L_INFO, "ret=0x%x " fmt_123 "",		\
		    (int)ret, __VA_ARGS__);			\
	}							\
        __zrt_log_pop_name(__func__);				\
    }


#define ZRT_LOG_STAT(v123, stat)					\
    ZRT_LOG(v123,							\
	    "st_dev=%lld,\n"						\
	    "st_ino=%lld,\n"						\
	    "nlink=%d,\n"						\
	    "st_mode=%s,\n"						\
	    "st_blksize=%d,\n"						\
	    "st_size=%lld,\n"						\
	    "st_blocks=%d,\n"						\
	    "st_atime=%lld,\n"						\
	    "st_mtime=%lld",						\
	    stat->st_dev, stat->st_ino, stat->st_nlink,			\
	    STR_STAT_ST_MODE(stat->st_mode), (int)stat->st_blksize,	\
	    stat->st_size, (int)stat->st_blocks, stat->st_atime, stat->st_mtime );


void __zrt_log_init();
const char* __zrt_log_syscall_stack_str();
void __zrt_log_push_name( const char* name );
void __zrt_log_pop_name( const char* name );
void __zrt_log_set_verbosity(int v);
int  __zrt_log_verbosity();
int  __zrt_log_fd();

/* 1 switch on logging
 * 0 switch off logging*/
void __zrt_log_enable(int status);
int  __zrt_log_is_enabled();
/* 1 enables safe log allowed during prolog state*/
/* 0 switch off prolog mode*/
void __zrt_log_prolog_mode_enable(int status);
int  __zrt_log_prolog_mode_is_enabled();

int __zrt_log_debug_get_buf(char **buf);
int32_t __zrt_log_write( int handle, const char* buf, int32_t size, int64_t offset);

#else
#define LOG_SYSCALL_START(fmt_123, ...)
#define LOG_SYSCALL_FINISH()

#endif

#endif /* ZRTLOG_H_ */
