/*
 * zrtlog.c
 *
 *  Created on: 18.09.2012
 *      Author: yaroslav
 */

#include "zvm.h"
#include "zrtlog.h"

#ifdef DEBUG
static int s_donotlog=0;
static int s_zrt_log_fd = -1;
static char s_logbuf[0x1000];

void enable_logging_current_syscall(){
    s_donotlog = 0; //switch on logging
}

void disable_logging_current_syscall(){
    s_donotlog = 1; //switch off logging
}

void setup_zrtlog_fd(int fd){
    s_zrt_log_fd = fd;
}

int32_t zrtlog_write( int handle, const char* buf, int32_t size, int64_t offset){
    return zvm_pwrite(handle, buf, size, offset);
}

int debug_handle_get_buf(char **buf){
    if ( s_donotlog != 0 ) return -1; /*switch off log for some functions*/
    *buf = s_logbuf;
    return s_zrt_log_fd;
}
#endif
