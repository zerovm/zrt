/*
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
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
static int s_log_enabled=1;  /*log enabled by default*/
static int s_log_prolog_mode_enabled=1; /*prolog mode enabled by default*/
static int s_zrt_log_fd = -1;
static int s_buffered_len = 0;
static char s_logbuf[LOG_BUFFER_SIZE];

static char s_nested_syscalls_str[MAX_NESTED_SYSCALL_LEN] = "\0";

/*it's accessing from zrtlogbase.h*/
LOG_BASE_ENABLE;

static void tfp_printf_putc(void* someobj, char ch){
    if ( s_buffered_len < LOG_BUFFER_SIZE )
	s_logbuf[s_buffered_len++] = ch;
}

void __zrt_log_init(const char* logpath){
    int i;
    for (i=0; i < MANIFEST->channels_count; i++ ){
	if ( !strcmp(logpath, MANIFEST->channels[i].name) ){
	    s_zrt_log_fd = i;
	    break;
	}
    }
    init_printf(NULL, tfp_printf_putc);
    s_log_enabled = 1;
    s_log_prolog_mode_enabled = 1;
}


void __zrt_log_enable(int status){
   s_log_enabled = status;
}

int  __zrt_log_is_enabled(){
    return s_log_enabled;
}

void __zrt_log_prolog_mode_enable(int status){
    s_log_prolog_mode_enabled = status;
}

int  __zrt_log_prolog_mode_is_enabled(){
    return s_log_prolog_mode_enabled;
}

void __zrt_log_set_verbosity(int v){
    s_verbosity_level = v;
}

int __zrt_log_verbosity(){
    return s_verbosity_level;
}

int __zrt_log_fd(){
    return s_zrt_log_fd;
}

const char* __zrt_log_syscall_stack_str(){
    return s_nested_syscalls_str;
}

void __zrt_log_push_name( const char* name ){
#ifdef LOG_FUNCTION_STACK_NAMES
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
#endif
}

void __zrt_log_pop_name( const char* expected_name ) {
#ifdef LOG_FUNCTION_STACK_NAMES
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
#endif
}

int32_t __zrt_log_write( int handle, const char* buf, int32_t size, int64_t offset){
    if ( s_log_prolog_mode_enabled ){ 
	if ( s_buffered_len ){
	    /*write to log buffer, that is using internally by tfp_printf*/
	    int wr = zvm_pwrite(handle, s_logbuf, s_buffered_len, offset);
	    s_buffered_len=0;
	    return wr;
	}
    }
    else
	return zvm_pwrite(handle, buf, size, offset);
    return 0;
}

int __zrt_log_debug_get_buf(char **buf){
    *buf = s_logbuf;
    return __zrt_log_fd();
}

#endif
