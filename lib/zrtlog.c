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
#define MAX_NESTED_SYSCALL_LEN 1000
static int s_verbosity_level = 1;
static int s_log_enabled=0;
static int s_zrt_log_fd = -1;
static char s_logbuf[LOG_BUFFER_SIZE];

static char s_nested_syscalls_str[MAX_NESTED_SYSCALL_LEN] = "\0";

void __zrt_log_enable(int status){
   s_log_enabled = status;
}

int  __zrt_log_is_enabled(){
    return s_log_enabled;
}

int __zrt_log_verbosity(){
    return s_verbosity_level;
}

void __zrt_log_set_fd(int fd){
    s_zrt_log_fd = fd;
    /*get verbosity level via environment*/
    const char* verbosity_str = getenv(VERBOSITY_ENV);
    if ( verbosity_str ){
	int verbosity = atoi(verbosity_str);
	if ( verbosity > 1)
	    s_verbosity_level = verbosity;
    }
}

int __zrt_log_fd(){
    return s_zrt_log_fd;
}

const char* __zrt_log_syscall_stack_str(){
    return s_nested_syscalls_str;
}

void __zrt_log_push_name( const char* name ){
    if ( !s_log_enabled ) return; /*logging switched off*/
    int len = strlen(name);
    if ( (strlen(s_nested_syscalls_str) + len + 3) < MAX_NESTED_SYSCALL_LEN ){
        snprintf( s_nested_syscalls_str + strlen(s_nested_syscalls_str), 
		  MAX_NESTED_SYSCALL_LEN-len,
		  " %s", name );
    }
    else{
	assert(0);
    }
}

void __zrt_log_pop_name( const char* expected_name ) {
    if ( !s_log_enabled ) return ; /*logging switched off*/
    char *s = strrchr(s_nested_syscalls_str, ' ');
    if ( s ){
	const char* actual_name = s+1;
	if (strcmp(expected_name, actual_name)){
	    ZRT_LOG(L_ERROR, "expected_name=%s, actual_name=%s", expected_name, actual_name);
	    /*check if popped name is equal to expectations*/
	    assert( !strcmp(expected_name, actual_name) ); 
	}
        s[0] = '\0';
    }
}

int32_t __zrt_log_write( int handle, const char* buf, int32_t size, int64_t offset){
    if ( !s_log_enabled ) return 0; /*logging switched off*/
    return zvm_pwrite(handle, buf, size, offset);
}

int __zrt_log_debug_get_buf(char **buf){
    if ( !s_log_enabled ) return -1; /*logging switched off*/
    *buf = s_logbuf;
    return s_zrt_log_fd;
}

#endif
