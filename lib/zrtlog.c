/*
 * zrtlog.c
 *
 *  Created on: 18.09.2012
 *      Author: yaroslav
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "zvm.h"
#include "zrtlog.h"

#ifdef DEBUG
#define MAX_NESTED_SYSCALLS_LOG 5
#define MAX_NESTED_SYSCALL_LEN 50
static int s_verbosity_level = 1;
static int s_donotlog=0;
static int s_zrt_log_fd = -1;
static char s_logbuf[LOG_BUFFER_SIZE];

static char s_nested_syscalls_str[MAX_NESTED_SYSCALL_LEN] = "\0";

void enable_logging_current_syscall(){
    //    s_donotlog = 0; //switch on logging
}

void disable_logging_current_syscall(){
    //    s_donotlog = 1; //switch off logging
}

int verbosity(){
    return s_verbosity_level;
}

void set_zrtlog_fd(int fd){
    s_zrt_log_fd = fd;
    /*get verbosity level via environment*/
    const char* verbosity_str = getenv(VERBOSITY_ENV);
    if ( verbosity_str ){
	int verbosity = atoi(verbosity_str);
	if ( verbosity > 1)
	    s_verbosity_level = verbosity;
    }
}

int zrtlog_fd(){
    return s_zrt_log_fd;
}

const char* syscall_stack_str(){
    return s_nested_syscalls_str;
}

void log_push_name( const char* name ){
    int len = strlen(name);
    if ( (strlen(s_nested_syscalls_str) + len + 3) < MAX_NESTED_SYSCALL_LEN ){
        snprintf( s_nested_syscalls_str + strlen(s_nested_syscalls_str), MAX_NESTED_SYSCALL_LEN-len,
                " %s", name );
    }
}

void log_pop_name( const char* name ) {
    char *s = strrchr(s_nested_syscalls_str, ' ');
    if ( s ){
        assert( !strcmp(name, s+1) ); /*check if popped name is equal to awaitings*/
        s[0] = '\0';
    }
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
