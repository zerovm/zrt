/*
 * zrtsyscalls.h
 * Entry point to whole syscalls, whose callbacks are received from zerovm
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

#ifndef ZRT_LIB_ZRTSYSCALLS_H
#define ZRT_LIB_ZRTSYSCALLS_H

#include "zvm.h"

#include <sys/stat.h> //mode_t

/*reserved channels list*/
#define DEV_STDIN  "/dev/stdin"
#define DEV_STDOUT "/dev/stdout"
#define DEV_STDERR "/dev/stderr"
#define DEV_FSTAB  "/dev/fstab"

#define UMASK_ENV "UMASK"


#include <time.h>   /*clock_t*/

/*forwards*/
struct stat;
struct dirent;
struct timeval;

struct MountsInterface;

/******************* zcalls_init_t functions **************/
/* irt basic *************************/
void zrt_zcall_exit(int status);
int zrt_zcall_gettod(struct timeval *tv);
int zrt_zcall_clock(clock_t *ticks);
int zrt_zcall_nanosleep(const struct timespec *req, struct timespec *rem);
int zrt_zcall_sched_yield(void);
int zrt_zcall_sysconf(int name, int *value);
/* irt fdio *************************/
int zrt_zcall_close(int fd);
int zrt_zcall_dup(int fd, int *newfd);
int zrt_zcall_dup2(int fd, int newfd);
int zrt_zcall_read(int fd, void *buf, size_t count, size_t *nread);
int zrt_zcall_write(int fd, const void *buf, size_t count, size_t *nwrote);
int zrt_zcall_seek(int fd, off_t offset, int whence, off_t *new_offset);
int zrt_zcall_fstat(int fd, struct stat *);
int zrt_zcall_getdents(int fd, struct dirent *, size_t count, size_t *nread);
/* irt filename *************************/
int zrt_zcall_open(const char *pathname, int oflag, mode_t cmode, int *newfd);
int zrt_zcall_stat(const char *pathname, struct stat *);
/* irt memory *************************/
int zrt_zcall_sysbrk(void **newbrk);
int zrt_zcall_mmap(void **addr, size_t len, int prot, int flags, int fd, off_t off);
int zrt_zcall_munmap(void *addr, size_t len);
/* irt dyncode *************************/
int zrt_zcall_dyncode_create(void *dest, const void *src, size_t size);
int zrt_zcall_dyncode_modify(void *dest, const void *src, size_t size);
int zrt_zcall_dyncode_delete(void *dest, size_t size);
/* irt thread *************************/
int zrt_zcall_thread_create(void *start_user_address, void *stack, void *thread_ptr);
void zrt_zcall_thread_exit(int32_t *stack_flag);
int zrt_zcall_thread_nice(const int nice);
/* irt mutex *************************/
int zrt_zcall_mutex_create(int *mutex_handle);
int zrt_zcall_mutex_destroy(int mutex_handle);
int zrt_zcall_mutex_lock(int mutex_handle);
int zrt_zcall_mutex_unlock(int mutex_handle);
int zrt_zcall_mutex_trylock(int mutex_handle);
/* irt cond *************************/
int zrt_zcall_cond_create(int *cond_handle);
int zrt_zcall_cond_destroy(int cond_handle);
int zrt_zcall_cond_signal(int cond_handle);
int zrt_zcall_cond_broadcast(int cond_handle);
int zrt_zcall_cond_wait(int cond_handle, int mutex_handle);
int zrt_zcall_cond_timed_wait_abs(int cond_handle, int mutex_handle,
			   const struct timespec *abstime);
/* irt tls *************************/
int zrt_zcall_tls_init(void *thread_ptr);
void *zrt_zcall_tls_get(void);
/* irt resource open *************************/
int zrt_zcall_open_resource(const char *file, int *fd);
/* irt clock *************************/
int zrt_zcall_getres(clockid_t clk_id, struct timespec *res);
int zrt_zcall_gettime(clockid_t clk_id, struct timespec *tp);


/************************** zcalls_zrt_t functions **************/
void zrt_zcall_zrt_setup(void);



/*****************************************************************
helpers, we should remove it from here */

/*get static object from zrtsyscalls.c*/
struct MountsInterface* transparent_mount();
/*move it from here*/
mode_t get_umask();
/*move it from here*/
mode_t apply_umask(mode_t mode);
/*move it from here*/
void debug_mes_stat(struct stat *stat);


#endif //ZRT_LIB_ZRTSYSCALLS_H
