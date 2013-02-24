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
#  include "zrtsyscalls.h"
static struct zcalls_init_t KZcalls_init = {
    zrt_zcall_exit,
    zrt_zcall_gettod,
    zrt_zcall_clock,
    zrt_zcall_nanosleep,
    zrt_zcall_sched_yield,
    zrt_zcall_sysconf,

    zrt_zcall_close,
    zrt_zcall_dup,
    zrt_zcall_dup2,
    zrt_zcall_read,
    zrt_zcall_write,
    zrt_zcall_seek,
    zrt_zcall_fstat,
    zrt_zcall_getdents,

    zrt_zcall_open,
    zrt_zcall_stat,

    zrt_zcall_sysbrk,
    zrt_zcall_mmap,
    zrt_zcall_munmap,

    zrt_zcall_dyncode_create,
    zrt_zcall_dyncode_modify,
    zrt_zcall_dyncode_delete,

    zrt_zcall_thread_create,
    zrt_zcall_thread_exit,
    zrt_zcall_thread_nice,

    zrt_zcall_mutex_create,
    zrt_zcall_mutex_destroy,
    zrt_zcall_mutex_lock,
    zrt_zcall_mutex_unlock,
    zrt_zcall_mutex_trylock,

    zrt_zcall_cond_create,
    zrt_zcall_cond_destroy,
    zrt_zcall_cond_signal,
    zrt_zcall_cond_broadcast,
    zrt_zcall_cond_wait,
    zrt_zcall_cond_timed_wait_abs,

    zrt_zcall_tls_init,
    zrt_zcall_tls_get,

    zrt_zcall_open_resource,

    zrt_zcall_getres,
    zrt_zcall_gettime
};

static struct zcalls_zrt_t KZcalls_zrt = {
    zrt_zcall_zrt_setup
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
    default:
	ret_type = -1;
	break;
    }
    return ret_type;
}
#endif //ZLIBC_STUB

