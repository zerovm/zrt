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


#ifndef ZRT_HELPER_MACROS_H_
#define ZRT_HELPER_MACROS_H_

#define MIN(a,b) (a < b ? a : b )
#define MAX(a,b) (a < b ? b : a )

#define CHECK_FLAG(flags, flag) ( (flags & (flag)) == (flag)? 1 : 0)
#define SET_ERRNO(err) {						\
	errno=err;							\
	ZRT_LOG( L_SHORT, "errno=%d, %s", errno, strerror(err) );	\
    }

/* ******************************************************************************
 * Syscallback mocks helper macroses*/
#define JOIN(x,y) x##y
#define ZRT_FUNC(x) JOIN(zrt_, x)

/* mock. replacing real syscall handler */
#define SYSCALL_MOCK(name_wo_zrt_prefix, code)				\
    static int32_t ZRT_FUNC(name_wo_zrt_prefix)(uint32_t *args)		\
    {									\
	LOG_SYSCALL_START("arg0=0x%X, arg1=0x%X, arg2=0x%X, arg3=0x%X, arg4=0x%X, arg5=0x%X", \
			  args[0], args[1], args[2], args[3], args[4], args[5]); \
	LOG_SHORT_SYSCALL_FINISH(code,P_TEXT,"");			\
	return code;							\
    }

/* ******************************************************************************
 * Syscalls stub for unsupported syscalls. It using an syscall index as argument
 * instead of SYSCALL_MOCK that require name as argument; the main goal of separated 
 * callback functions it's differing non implemented syscalls received by zrt. 
 * Using a single hanler for that syscalls makes impossible retrieval of syscall id. */

#define NON_IMPLEMENTED_PREFIX zrt_nan_

/*Generate function name, intnded for using in zrt_syscalls array*/
#define SYSCALL_NOT_IMPLEMENTED(number) JOIN(NON_IMPLEMENTED_PREFIX,number)

/*every SYSCALL_NOT_IMPLEMENTED(number) should have appropriate stub, 
 * where 'number' is syscall index*/
#define SYSCALL_STUB_IMPLEMENTATION(number)				\
    static int32_t JOIN(NON_IMPLEMENTED_PREFIX,number)(uint32_t *args) { \
	LOG_SYSCALL_START("arg0=0x%X, arg1=0x%X, arg2=0x%X, arg3=0x%X, arg4=0x%X, arg5=0x%X", \
			  args[0], args[1], args[2], args[3], args[4], args[5]); \
	LOG_SHORT_SYSCALL_FINISH(0,P_TEXT,"");				\
	return 0;							\
    }


/* ******************************************************************************
 * Syscallback input validate helpers*/

/*Validate syscall input parameter*/
#define VALIDATE_SYSCALL_PTR(arg_pointer)			\
    if ( arg_pointer == NULL ) {				\
	errno=EFAULT;						\
	LOG_SHORT_SYSCALL_FINISH(-1,"Bad pointer %s=%p",	\
				 #arg_pointer, arg_pointer);	\
	return -1;						\
    }
/*Validate syscall input parameter of substituted glibc syscall*/
#define VALIDATE_SUBSTITUTED_SYSCALL_PTR(arg_pointer) {		\
        char str[20];						\
        sprintf(str, "%p", arg_pointer);			\
        if ( arg_pointer == NULL || !strcmp(str, "(nil)") ) {	\
            errno=EFAULT;					\
	    LOG_SHORT_SYSCALL_FINISH(-1,P_TEXT,"");		\
            return -1;						\
        }							\
    }


/*alloc and copy null-terminated string into text*/
#define STR_ALLOCA_COPY(str) strcpy((char*)alloca(strlen(str)+1), str)

#ifndef strndupa
# define strndupa(s, n)					\
    (__extension__					\
     ({							\
	 int __len = (n);				\
	 char *__old = (char *)	(s);			\
	 char *__new = (char *) alloca (__len + 1);	\
	 __new[__len] = '\0';				\
	 (char *) memcpy (__new, __old, __len);		\
     }))
#endif //strndupa

/*alloc in stack null terminated string*/
#define GET_STRING(str,size) strndupa( (str), (size))

#define APPLY_UMASK(mode_p){				\
	if ( getenv(UMASK_ENV) != NULL ){		\
	    mode_t umask;				\
	    sscanf( getenv(UMASK_ENV), "%o", &umask);	\
	    ZRT_LOG(L_INFO, "umask=%o", umask);		\
	    *mode_p = ~umask & *mode_p;			\
	}						\
    }

#define PROLOG_WARNING "Can not run - ZRT not ready"

#endif /* ZRT_HELPER_MACROS_H_ */
