/*
 * zrtlog.c
 *
 *  Created on: 18.09.2012
 *      Author: yaroslav
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "zvm.h"
#include "zrtlog.h"

#ifdef DEBUG
#define MAX_NESTED_SYSCALLS_LOG 5
#define MAX_NESTED_SYSCALL_LEN 50
static int s_donotlog=0;
static int s_zrt_log_fd = -1;
static char s_logbuf[0x1000];

static char s_nested_syscalls_str[MAX_NESTED_SYSCALL_LEN] = "\0";

void enable_logging_current_syscall(){
    s_donotlog = 0; //switch on logging
}

void disable_logging_current_syscall(){
    s_donotlog = 1; //switch off logging
}

void setup_zrtlog_fd(int fd){
    s_zrt_log_fd = fd;
}

const char* syscall_stack_str(){
    return s_nested_syscalls_str;
}

void syscall_push( const char* name ){
    int len = strlen(name);
    if ( (strlen(s_nested_syscalls_str) + len + 3) < MAX_NESTED_SYSCALL_LEN ){
        snprintf( s_nested_syscalls_str + strlen(s_nested_syscalls_str), MAX_NESTED_SYSCALL_LEN-len,
                " %s", name );
        //memcpy( s_nested_syscalls_str + strlen(s_nested_syscalls_str), " \0", 2 ); //copy space and null char
        //memcpy( s_nested_syscalls_str + strlen(s_nested_syscalls_str), name, len );//copy name
        //memcpy( s_nested_syscalls_str + len, "\0", 1 ); //copy null char
    }
}

void syscall_pop(){
    char *s = strrchr(s_nested_syscalls_str, ' ');
    if ( s ){
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
