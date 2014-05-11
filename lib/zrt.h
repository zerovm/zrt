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

#define UMASK_ENV "UMASK"

/*forwards*/
struct stat;
struct dirent;
struct timeval;
struct timespec;
struct NvramLoader;


#define ZCALLS_INIT 1   /*use as type param in __query_zcalls*/
struct zcalls_init_t{
    /*zcalls initializer, coming just after __zcalls_query*/
    void (*init)(void);
    /* irt basic *************************/
    void (*exit)(int status);
    int (*gettod)(struct timeval *tv);
    int (*clock)(clock_t *ticks);
    int (*nanosleep)(const struct timespec *req, struct timespec *rem);
    int (*sched_yield)(void);
    int (*sysconf)(int name, int *value);
    /* irt fdio *************************/
    int (*close)(int fd);
    int (*dup)(int fd);
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
    /* another ***************************/
    int (*chdir)(const char *path);
};

#define ZCALLS_ZRT 2         /*use as type param in __query_zcalls*/
struct zcalls_zrt_t{
    void (*zrt_setup)(void);
    void (*zrt_premain)(void);
    void (*zrt_postmain)(int exitcode);
};

#define ZCALLS_NONSYSCALLS 3 /*use as type param in __query_zcalls*/
struct zcalls_nonsyscalls_t{
    int  (*fcntl) (int fd, int cmd, ...);
    int  (*link)(const char *oldpath, const char *newpath);
    int  (*unlink)(const char *pathname);
    int  (*rmdir)(const char *pathdir);
    int  (*mkdir)(const char *pathdir, mode_t mode);
    int  (*chmod)(const char *path, mode_t mode);
    int  (*fchmod)(int fd, mode_t mode);
    int  (*chown)(const char *path, uid_t owner, gid_t group);
    int  (*fchown)(int fd, uid_t owner, gid_t group);
    int  (*ftruncate)(int fd, off_t length);
    int  (*stat_realpath) (const char *abspathname, struct stat *stat);
    int (*get_phys_pages)(void);
    int (*get_avphys_pages)(void);
    int (*fchdir)(int fd);
};

#define ZCALLS_ENV_ARGS_INIT 4         /*use as type param in __query_zcalls*/
struct zcalls_env_args_init_t{
    /*read&parse nvram and get lengths and count, for both envs, args 
     *to know what memories need to allocate on prolog side*/
    void (*read_nvram_get_args_envs)( int *args_buf_size, 
				      int *envs_buf_size, int *env_count );
    /*copy args &envs from zrt into preallocated arrays on prolog side*/
    void (*get_nvram_args_envs)( char** args, char* args_buf, int args_buf_size,
				 char** envs, char* envs_buf, int envs_buf_size );
};

/*Part of LIBC, used for ZRT initialization;
 *@return retrieved table type, should be same as requested*/
int
__query_zcalls(int type, void** table );

/*Part of LIBC, linking it and GLIBC directly with no use ZCALLS
 interface; it is a workaround for toolchain build error "undefined
 reference" that appeared for functions sources resides in glibc/posix
 folder.*/
extern int __nacl_irt_pread(int fd, void *buf, int count, long long offset,
			    int *nread);

/*Part of LIBC, linking it and GLIBC directly with no use ZCALLS
 interface; it is a workaround for toolchain build error "undefined
 reference" that appeared for functions sources resides in glibc/posix
 folder.*/
extern int __nacl_irt_pwrite(int fd, const void *buf, int count, long long offset,
			     int *nwrote);

#endif /* _LIB_ZRT_H_ */
