/*
 * zrt_helper_macros.h
 *
 *  Created on: 07.10.2012
 *      Author: yaroslav
 */

#ifndef ZRT_HELPER_MACROS_H_
#define ZRT_HELPER_MACROS_H_

#define CHECK_FLAG(flags, flag) ( (flags & (flag)) == (flag)? 1 : 0)
#define SET_ERRNO(err) {errno=err;ZRT_LOG_ERRNO(errno);}

/* ******************************************************************************
 * Syscallbacks debug macros*/
#ifdef DEBUG

/* /\*tests tests tests tests tests tests tests tests tests tests tests tests *\/ */
/* #define LOG_VARP(name) {	 */
/* 	char *buf__123;							\ */
/* 	int debug_handle = debug_handle_get_buf(&buf__123);		\ */
/* 	len = snprintf(buf__123, LOG_BUFFER_SIZE, "%", 0 );	\ */
/* 	zrtlog_write(debug_handle, buf__123, len, 0);			\ */

/* 	for (i_123 = 0; i_123 >= 0; i_123 = va_arg(ap_123, int32_t)){ \ */
/* 	    ZRT_LOG(L_SHORT, "arg#%d=%x ", counter_123++, i_123);	\ */
/* 	}								\ */
/* 	va_end();							\ */
/*     } */
/* /\*tests tests tests tests tests tests tests tests tests tests tests tests *\/ */


/* Push current NACL syscall into logging stack that printing for every log invocation.
 * Enable logging for NACL syscall, and printing arguments*/
#define LOG_SYSCALL_START(args_p, args_c) {					\
	uint32_t* args_123 = (uint32_t*)args_p;				\
	log_push_name(__func__);					\
	enable_logging_current_syscall();				\
	if ( args_p == NULL ) {						\
	    ZRT_LOG(L_SHORT, "%s", __func__);				\
	}								\
	else {								\
	    char *buf__123;						\
	    int debug_handle = debug_handle_get_buf(&buf__123);		\
	    int i;							\
	    int len = snprintf(buf__123, LOG_BUFFER_SIZE,		\
			       "%s; [%s]; syscall, %d:",		\
			       __FILE__,syscall_stack_str(),__LINE__ ); \
	    for ( i=0; i < args_c; i++ ){				\
		len += snprintf(buf__123+len, LOG_BUFFER_SIZE-len,	\
				" arg[%d]=0x%X", i, args_123[i] );	\
	    }								\
	    len +=snprintf(buf__123+len, LOG_BUFFER_SIZE-len, "\n" );	\
	    zrtlog_write(debug_handle, buf__123, len, 0);		\
	    (void)len;							\
	}								\
    }									\

/* Pop from logging stack current syscall function.
 * Prints retcode and errno*/
#define LOG_SYSCALL_FINISH(ret){					\
        if ( ret < 0 ){							\
	    ZRT_LOG(L_SHORT, "ret=0x%x, errno=%d", (int)ret, errno);	\
	}								\
        else{								\
	    ZRT_LOG(L_SHORT, "ret=0x%x", (int)ret);			\
	}								\
        log_pop_name(__func__);						\
    }

#else
#define LOG_SYSCALL_START(args_p,args_c)
#define LOG_SYSCALL_FINISH()
#endif


/* ******************************************************************************
 * Syscallback mocks helper macroses*/
#define JOIN(x,y) x##y
#define ZRT_FUNC(x) JOIN(zrt_, x)

/* mock. replacing real syscall handler */
#define SYSCALL_MOCK(name_wo_zrt_prefix, code)			\
    static int32_t ZRT_FUNC(name_wo_zrt_prefix)(uint32_t *args)	\
    {								\
	LOG_SYSCALL_START(args,6);				\
	LOG_SYSCALL_FINISH(code);				\
	return code;						\
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
	LOG_SYSCALL_START(args,6);					\
	LOG_SYSCALL_FINISH(0);						\
	return 0;							\
    }


/* ******************************************************************************
 * Syscallback input validate helpers*/

/*Validate syscall input parameter*/
#define VALIDATE_SYSCALL_PTR(arg_pointer)				\
    if ( arg_pointer == NULL ) {					\
	ZRT_LOG(L_SHORT, "Bad pointer %s=%p", #arg_pointer, arg_pointer); \
	errno=EFAULT;							\
	LOG_SYSCALL_FINISH(-1);						\
	return -1;							\
    }
/*Validate syscall input parameter of substituted glibc syscall*/
#define VALIDATE_SUBSTITUTED_SYSCALL_PTR(arg_pointer) {		\
        char str[20];						\
        sprintf(str, "%p", arg_pointer);			\
        if ( arg_pointer == NULL || !strcmp(str, "(nil)") ) {	\
            errno=EFAULT;					\
            LOG_SYSCALL_FINISH(-1);				\
            return -1;						\
        }							\
    }


#endif /* ZRT_HELPER_MACROS_H_ */
