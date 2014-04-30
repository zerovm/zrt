/*
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "zrtapi.h"
#include "zrt_defines.h"
#include "channels_reserved.h"

#define START_TRACE        "START"
#define FUNCTION_ENTRY_STR "enter"
#define FUNCTION_EXIT_STR  "exit"
#define END_TRACE          "EXIT"

typedef enum { FUNCTION_ENTRY, FUNCTION_EXIT } entry_t;
typedef enum { A_UNINITIALIZED, A_DISABLED, A_ACTIVE, A_INACTIVE } active_t;

/* Trace initialization */
FILE *
__NON_INSTRUMENT_FUNCTION__
ptrace_init()
{
    FILE *ret = fopen(DEV_TRACE, "a");
    if(ret == NULL)
	return NULL;

    fprintf(ret, START_TRACE "\n");
    fflush(ret);

    return ret;
}

/* Function called by every function event */
void
__NON_INSTRUMENT_FUNCTION__
ptrace(entry_t e, void *p)
{
    static active_t active = A_UNINITIALIZED;
    static FILE *f = NULL;

    switch(active) {
    case A_INACTIVE:
    case A_DISABLED:
	return;
    case A_ACTIVE:
	active = A_DISABLED;
	break;
    case A_UNINITIALIZED:
	active = A_DISABLED;
	if((f = ptrace_init()) == NULL)
	    return;
	break;
    }

    switch(e) {
    case FUNCTION_ENTRY:
	fprintf(f, "%s %p\n", FUNCTION_ENTRY_STR, p);
	break;
    case FUNCTION_EXIT:
	fprintf(f, "%s %p\n", FUNCTION_EXIT_STR, p);
	break;
    }
    fflush(f);
    active = A_ACTIVE;

    return;
}

/* According to gcc documentation: called upon function entry */
void
__NON_INSTRUMENT_FUNCTION__
__cyg_profile_func_enter(void *this_fn, void *call_site)
{
    if ( is_ptrace_allowed() ) {
	int backup_errno = errno;
	ptrace(FUNCTION_ENTRY, this_fn);
	errno = backup_errno;
    }
    (void) call_site;
}

/* According to gcc documentation: called upon function exit */
void
__NON_INSTRUMENT_FUNCTION__
__cyg_profile_func_exit(void *this_fn, void *call_site)
{
    if ( is_ptrace_allowed() ) {
	int backup_errno = errno;
	ptrace(FUNCTION_EXIT, this_fn);
	errno = backup_errno;
    }
    (void) call_site;
}

