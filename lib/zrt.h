/*
 * zrt.h
 * ZeroVM runtime, it's not required to include this file into user code
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

#ifndef _LIB_ZRT_H_
#define _LIB_ZRT_H_

#include <time.h>   /*clock_t*/
#include <stdint.h>
#include <sys/types.h>


/*forwards*/
struct stat;
struct dirent;
struct timeval;

#define ZCALLS_INIT 1
struct zcalls_init_t{
    /* irt basic *************************/
    void (*exit)(int status);
    int (*gettod)(struct timeval *tv);
    int (*clock)(clock_t *ticks);
    int (*nanosleep)(const struct timespec *req, struct timespec *rem);
    int (*sched_yield)(void);
    int (*sysconf)(int name, int *value);
    /* irt fdio *************************/
    int (*close)(int fd);
    int (*dup)(int fd, int *newfd);
    int (*dup2)(int fd, int newfd);
    int (*read)(int fd, void *buf, size_t count, size_t *nread);
    int (*write)(int fd, const void *buf, size_t count, size_t *nwrote);
    int (*seek)(int fd, off_t offset, int whence, off_t *new_offset);
    int (*fstat)(int fd, struct stat *);
    int (*getdents)(int fd, struct dirent *, size_t count, size_t *nread);
    /* irt filename *************************/
    int (*open)(const char *pathname, int oflag, mode_t cmode, int *newfd);
    int (*stat)(const char *pathname, struct stat *);
    /* irt memory *************************/
    int (*sysbrk)(void **newbrk);
    int (*mmap)(void **addr, size_t len, int prot, int flags, int fd, off_t off);
    int (*munmap)(void *addr, size_t len);
    /* irt dyncode *************************/
    int (*dyncode_create)(void *dest, const void *src, size_t size);
    int (*dyncode_modify)(void *dest, const void *src, size_t size);
    int (*dyncode_delete)(void *dest, size_t size);
    /* irt thread *************************/
    int (*thread_create)(void *start_user_address, void *stack, void *thread_ptr);
    void (*thread_exit)(int32_t *stack_flag);
    int (*thread_nice)(const int nice);
    /* irt mutex *************************/
    int (*mutex_create)(int *mutex_handle);
    int (*mutex_destroy)(int mutex_handle);
    int (*mutex_lock)(int mutex_handle);
    int (*mutex_unlock)(int mutex_handle);
    int (*mutex_trylock)(int mutex_handle);
    /* irt cond *************************/
    int (*cond_create)(int *cond_handle);
    int (*cond_destroy)(int cond_handle);
    int (*cond_signal)(int cond_handle);
    int (*cond_broadcast)(int cond_handle);
    int (*cond_wait)(int cond_handle, int mutex_handle);
    int (*cond_timed_wait_abs)(int cond_handle, int mutex_handle,
			       const struct timespec *abstime);
    /* irt tls *************************/
    int (*tls_init)(void *thread_ptr);
    void *(*tls_get)(void);
    /* irt resource open *************************/
    int (*open_resource)(const char *file, int *fd);
    /* irt clock *************************/
    int (*getres)(clockid_t clk_id, struct timespec *res);
    int (*gettime)(clockid_t clk_id, struct timespec *tp);
};

#define ZCALLS_ZRT 2
struct zcalls_zrt_t{
    void (*zrt_setup)(void);
};


/*Part of ZLIBC, used for ZRT initialization;
 *@return retrieved table type, should be same as requested*/
int
__query_zcalls(int type, void** table );


#endif /* _LIB_ZRT_H_ */
