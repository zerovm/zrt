/*
 * malloc report, it used to get addresses of non freed memory
 * allocated by malloc, realloc
 *
 * Copyright (c) 2014, LiteStack, Inc.
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

#ifdef ALLOC_REPORT

#include <stdio.h>
#include <malloc.h>
#include <execinfo.h>

#include "zrt_defines.h"
#include "zrtapi.h"
#include "fs/channels_reserved.h"

#define POINTERS_ARRAY_CAPACITY 10000
#define STR_TRACE_MAX_LEN 300
#define BACKTRACE_ARRAY_SIZE 30

typedef struct {void *pointer; char bt[STR_TRACE_MAX_LEN];} hook_pointer_t;

#define SAVE_EXISTING_HOOKS			\
    /* Save underlying hooks */			\
    s_old_malloc_hook = __malloc_hook;		\
    s_old_realloc_hook = __realloc_hook;	\
    s_old_free_hook = __free_hook;

#define UNSET_HOOKS				\
    /* Restore all old hooks */			\
    __malloc_hook = s_old_malloc_hook;		\
    __realloc_hook = s_old_realloc_hook;	\
    __free_hook = s_old_free_hook;

#define SET_HOOKS				\
    /* Restore our own hooks */			\
    __malloc_hook = new_malloc_hook;		\
    __realloc_hook = new_realloc_hook;		\
    __free_hook = new_free_hook;

#define SET_HOOKS_SAVE_EXISTING			\
    SAVE_EXISTING_HOOKS;			\
    SET_HOOKS;

     
void report() __DESTRUCTOR_FUNCTION__;

/* Prototypes for our hooks.  */
static void hook_init (void);
static void *new_malloc_hook (size_t, const void *);
static void *new_realloc_hook (void *ptr, size_t size, const void *caller);
static void new_free_hook (void*, const void *);

/* Override initializing hook from the C library. */
void (*__malloc_initialize_hook) (void) = hook_init;

//static variables
static void *s_old_malloc_hook;
static void *s_old_realloc_hook;
static void *s_old_free_hook ;
static hook_pointer_t s_pointers_array[POINTERS_ARRAY_CAPACITY];
static FILE *s_report_file;


const char *print_backtrace(char* buf, int buflen){
    /*Allow to get backtrace only when entered main, due to errors at
      prolog, exit*/
    if ( !is_user_main_executing() ) return NULL;
    void *backtrace_pointers[BACKTRACE_ARRAY_SIZE];
    char **backtrace_names;
    int count_addresses = backtrace ((void**)&backtrace_pointers, 
				     sizeof(backtrace_pointers)/sizeof(*backtrace_pointers));
    backtrace_names = backtrace_symbols (backtrace_pointers, count_addresses);

    int offset=0;
    int i=0;
    for (; i<count_addresses&&(buflen-offset)>0; i++ ){
	int res =snprintf(buf+offset, buflen-offset, ", %s", backtrace_names[i]);
	if ( res == -1 ) break;
	offset+=res;
    }
    free(backtrace_names);
    return buf;
}



hook_pointer_t *search_pointer(const void* p){
    int i=0;
    for (; i < POINTERS_ARRAY_CAPACITY; i++){
	if ( p == s_pointers_array[i].pointer )
	    return &s_pointers_array[i];
    }
    return NULL;
}

void report(){
    int i=0;
    if ( s_report_file ){
	for (; i < POINTERS_ARRAY_CAPACITY; i++)
	    if ( s_pointers_array[i].pointer != NULL ){ 
		fprintf(s_report_file, "ALLOC POINTER(%d) %p, %s\n",
			i,
			s_pointers_array[i].pointer, 
			s_pointers_array[i].bt );
	    }
	fflush(s_report_file);
	fclose(s_report_file);
    }
}

void hook_init(){
    s_report_file = fopen(DEV_ALLOC_REPORT, "w");
    /*if set hooks only if report file exist*/
    if ( s_report_file ){
	SET_HOOKS_SAVE_EXISTING;
    }
}

static void *
new_malloc_hook (size_t size, const void *caller){
    void *result;
    UNSET_HOOKS;
    /* Call recursively */
    result = malloc (size);
    if ( is_user_main_executing() ){
	hook_pointer_t *p;
	if ( (p=search_pointer(NULL)) != NULL){
	    p->pointer = result;
	    p->bt[0]='\0';
	    print_backtrace(p->bt, sizeof(p->bt));
	}
    }
    SET_HOOKS_SAVE_EXISTING;
    return result;
}
     
static void *
new_realloc_hook(void *ptr, size_t size, const void *caller){
    void *result;
    /* Restore all old hooks */
    UNSET_HOOKS;
    /* Call recursively */
    result = realloc (ptr, size);
    if ( is_user_main_executing() ){
	hook_pointer_t *p;
	//remove old addr
	if ( (p=search_pointer(ptr)) != NULL){
	    p->pointer = NULL;
	}
	//save new addr
	if ( (p=search_pointer(NULL)) != NULL){
	    p->pointer = result;
	    p->bt[0]='\0';
	    print_backtrace(p->bt, sizeof(p->bt));
	}
    }
    SET_HOOKS_SAVE_EXISTING;
    return result;
}

static void
new_free_hook (void *ptr, const void *caller){
    /* Restore all old hooks */
    UNSET_HOOKS;
    /* Call recursively */
    free (ptr);
    if ( is_user_main_executing() ){
	hook_pointer_t *p;
	if ( (p=search_pointer(ptr)) != NULL){
	    p->pointer = NULL;
	}
    }
    SET_HOOKS_SAVE_EXISTING;
}

#endif //ALLOC_REPORT
