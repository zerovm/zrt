/*
 *  Here is defined query_zcalls entry point to ZRT code from ZLIBC.
 *  The zcall interfaces initialization does here;
 *  Created on: Feb 22, 2013
 *      Author: YaroslavLitvinov
 */

//#define ZLIBC_STUB
/* Define ZLIBC_STUB to create empty implementation of zcalls interface.
 * It's needed while building ZLIBC to cut off the rest of ZRT library code.
 * For generic ZRT using it is should no be defined;*/


#ifndef ZLIBC_STUB
#  include "zrt.h"
#  include "zcalls.h"

static struct zcalls_init_t KZcalls_init = {
    zrt_zcall_prolog_init,
    zrt_zcall_prolog_exit,
    zrt_zcall_prolog_gettod,
    zrt_zcall_prolog_clock,
    zrt_zcall_prolog_nanosleep,
    zrt_zcall_prolog_sched_yield,
    zrt_zcall_prolog_sysconf,

    zrt_zcall_prolog_close,
    zrt_zcall_prolog_dup,
    zrt_zcall_prolog_dup2,
    zrt_zcall_prolog_read,
    zrt_zcall_prolog_write,
    zrt_zcall_prolog_seek,
    zrt_zcall_prolog_fstat,
    zrt_zcall_prolog_getdents,

    zrt_zcall_prolog_open,
    zrt_zcall_prolog_stat,

    zrt_zcall_prolog_sysbrk,
    zrt_zcall_prolog_mmap,
    zrt_zcall_prolog_munmap,

    zrt_zcall_prolog_dyncode_create,
    zrt_zcall_prolog_dyncode_modify,
    zrt_zcall_prolog_dyncode_delete,

    zrt_zcall_prolog_thread_create,
    zrt_zcall_prolog_thread_exit,
    zrt_zcall_prolog_thread_nice,

    zrt_zcall_prolog_mutex_create,
    zrt_zcall_prolog_mutex_destroy,
    zrt_zcall_prolog_mutex_lock,
    zrt_zcall_prolog_mutex_unlock,
    zrt_zcall_prolog_mutex_trylock,

    zrt_zcall_prolog_cond_create,
    zrt_zcall_prolog_cond_destroy,
    zrt_zcall_prolog_cond_signal,
    zrt_zcall_prolog_cond_broadcast,
    zrt_zcall_prolog_cond_wait,
    zrt_zcall_prolog_cond_timed_wait_abs,

    zrt_zcall_prolog_tls_init,
    zrt_zcall_prolog_tls_get,

    zrt_zcall_prolog_open_resource,

    zrt_zcall_prolog_getres,
    zrt_zcall_prolog_gettime,

    zrt_zcall_prolog_chdir
};

static struct zcalls_zrt_t KZcalls_zrt = {
    zrt_zcall_prolog_zrt_setup
};

static struct zcalls_env_args_init_t KZcalls_env_args_init = {
    zrt_zcall_prolog_nvram_read_get_args_envs,
    zrt_zcall_prolog_nvram_get_args_envs
};

static struct zcalls_nonsyscalls_t KZcalls_nonsyscalls = {
    zrt_zcall_loglibc,
    zrt_zcall_fcntl,
    zrt_zcall_link,
    zrt_zcall_unlink,
    zrt_zcall_rmdir,
    zrt_zcall_mkdir,
    zrt_zcall_chmod,
    zrt_zcall_fchmod,
    zrt_zcall_chown,
    zrt_zcall_fchown,
    zrt_zcall_ftruncate
};
#endif //ZLIBC_STUB


int
__query_zcalls(int type, void** table )
#ifdef ZLIBC_STUB
{  return -1; }
#else
{
    int ret_type = -1;
    switch( type ){
    case ZCALLS_INIT:
	*table = &KZcalls_init;
	ret_type = type;
	break;
    case ZCALLS_ZRT:
	*table = &KZcalls_zrt;
	ret_type = type;
	break;
    case ZCALLS_NONSYSCALLS:
	*table = &KZcalls_nonsyscalls;
	ret_type = type;
	break;
    case ZCALLS_ENV_ARGS_INIT:
    	*table = &KZcalls_env_args_init;
    	ret_type = type;
    	break;
    default:
	ret_type = -1;
	break;
    }
    return ret_type;
}
#endif //ZLIBC_STUB

