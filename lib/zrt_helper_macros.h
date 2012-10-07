/*
 * zrt_helper_macros.h
 *
 *  Created on: 07.10.2012
 *      Author: yaroslav
 */

#ifndef ZRT_HELPER_MACROS_H_
#define ZRT_HELPER_MACROS_H_

/* ******************************************************************************
 * Syscallbacks debug macros*/
#ifdef DEBUG
/* Push current NACL syscall into logging stack that printing for every log invocation.
 * Enable logging for NACL syscall, and printing arguments*/
#define LOG_SYSCALL_START(args_p) {\
        uint32_t* args_123 = (uint32_t*)args_p;\
        log_push_name(__func__);\
        enable_logging_current_syscall(); \
        if ( args_p == NULL ) {\
            zrt_log("%s", __func__); \
        }\
        else {\
            zrt_log("syscall arg[0]=0x%x, arg[1]=0x%x, arg[2]=0x%x, arg[3]=0x%x, arg[4]=0x%x", \
                    args_123[0], args_123[1], args_123[2], args_123[3], args_123[4] ); \
        }\
}
/* Pop from logging stack current syscall function.
 * Prints retcode and errno*/
#define LOG_SYSCALL_FINISH(ret){ \
        if ( ret == -1 ){ zrt_log("ret=0x%x, errno=%d", (int)ret, errno);} \
        else{ zrt_log("ret=0x%x", (int)ret);} \
        log_pop_name(__func__); \
    }

#else
#define LOG_SYSCALL_START(args_p)
#define LOG_SYSCALL_FINISH()
#endif


/* ******************************************************************************
 * Syscallback mocks helper macroses*/
#define JOIN(x,y) x##y
#define ZRT_FUNC(x) JOIN(zrt_, x)

/* mock. replacing real syscall handler */
#define SYSCALL_MOCK(name_wo_zrt_prefix, code) \
        static int32_t ZRT_FUNC(name_wo_zrt_prefix)(uint32_t *args)\
        {\
    LOG_SYSCALL_START(args);\
    LOG_SYSCALL_FINISH(code);\
    return code; \
        }

/* ******************************************************************************
 * Syscallback input validate helpers*/

/*Validate syscall input parameter*/
#define VALIDATE_SYSCALL_PTR(arg_pointer) \
        if ( arg_pointer == NULL ) {\
            zrt_log("Bad pointer %s=%p", #arg_pointer, arg_pointer);\
            errno=EFAULT;\
            LOG_SYSCALL_FINISH(-1);\
            return -1;\
        }
/*Validate syscall input parameter of substituted glibc syscall*/
#define VALIDATE_SUBSTITUTED_SYSCALL_PTR(arg_pointer) { \
        char str[20]; \
        sprintf(str, "%p", arg_pointer); \
        if ( arg_pointer == NULL || !strcmp(str, "(nil)") ) {\
            errno=EFAULT; \
            LOG_SYSCALL_FINISH(-1);\
            return -1;\
        }\
}


#endif /* ZRT_HELPER_MACROS_H_ */
