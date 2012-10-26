/*
 * zrtlog.h
 *
 *  Created on: 18.09.2012
 *      Author: yaroslav
 */

#ifndef ZRTLOG_H_
#define ZRTLOG_H_

#include "stdint.h"

#ifndef DEBUG
#define DEBUG
#endif

#ifdef DEBUG
#define LOG_BUFFER_SIZE 0x1000
#define zrt_log(fmt, ...) \
        do {\
            char *buf__123; \
            int debug_handle = debug_handle_get_buf(&buf__123); \
            int len;\
            if(debug_handle < 0) break;\
            len = snprintf(buf__123, LOG_BUFFER_SIZE, "%s; [%s]; %s, %d: " fmt "\n", \
            __FILE__, syscall_stack_str(), __func__, __LINE__, __VA_ARGS__);\
            zrtlog_write(debug_handle, buf__123, len, 0); \
        } while(0)

#define zrt_log_str(text) \
        do {\
            char *buf__123; \
            int debug_handle = debug_handle_get_buf(&buf__123); \
            int len;\
            if( debug_handle < 0) break;\
            len = snprintf(buf__123, LOG_BUFFER_SIZE, "%s; [%s]; %s, %d: %s\n", \
            __FILE__, syscall_stack_str(), __func__, __LINE__, text);\
            zrtlog_write(debug_handle, buf__123, len, 0); \
        } while(0)

#define ZRT_LOG_ERRNO( errcode ) \
    {\
	zrt_log( "errno=%d, %s", errcode, strerror(errcode) );	\
    }


#define ZRT_LOG_DELIMETER \
        do {\
            char *buf__123; \
            int debug_handle = debug_handle_get_buf(&buf__123); \
            int len;\
            if( debug_handle < 0) break;\
            len = snprintf(buf__123, LOG_BUFFER_SIZE, "%060d\n", 0 );\
            zrtlog_write(debug_handle, buf__123, len, 0); \
        } while(0)


const char* syscall_stack_str();
void log_push_name( const char* name );
void log_pop_name( const char* name );
void set_zrtlog_fd(int fd);
int zrt_log_fd();
void enable_logging_current_syscall();
void disable_logging_current_syscall();
int debug_handle_get_buf(char **buf);
int32_t zrtlog_write( int handle, const char* buf, int32_t size, int64_t offset);

#endif

#endif /* ZRTLOG_H_ */
