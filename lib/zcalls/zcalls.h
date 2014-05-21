/*
 *
 * Copyright (c) 2013, LiteStack, Inc.
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

#ifndef __ZCALLS_H__
#define __ZCALLS_H__

#include <time.h>   /*clock_t*/

#include "zrt.h"
#include "zrt_defines.h"

/*forwards*/
struct stat;
struct dirent;
struct timeval;

struct NvramLoader;

/***************************************************************************
* Declaration of ZLIBC syscall implementations that used as main syscall 
* handlers while prolog initializing. Function with prefix zrt_zcall_prolog_
* should be used for initialization of struct zcalls_init_t in __query_zcalls.
***************************************************************************/

/*zrt internals has 3 stages of startup: running prolog, zrt init, zrt setup
*@return 1 if zrt init OK*/
int is_zrt_ready();

/******************* zcalls_init_t functions **************/
void zrt_zcall_prolog_init(void);
/* irt basic *************************/
void zrt_zcall_prolog_exit(int status);
int zrt_zcall_prolog_gettod(struct timeval *tv);
int zrt_zcall_prolog_clock(clock_t *ticks);
int zrt_zcall_prolog_nanosleep(const struct timespec *req, struct timespec *rem);
int zrt_zcall_prolog_sched_yield(void);
/* irt fdio *************************/
int zrt_zcall_prolog_close(int fd);
int zrt_zcall_prolog_dup(int fd);
int zrt_zcall_prolog_dup2(int fd, int newfd);
int zrt_zcall_prolog_read(int fd, void *buf, size_t count, size_t *nread);
int __NON_INSTRUMENT_FUNCTION__
zrt_zcall_prolog_write(int fd, const void *buf, size_t count, size_t *nwrote);
int __NON_INSTRUMENT_FUNCTION__
zrt_zcall_prolog_pread(int fd, void *buf, size_t count, off_t offset, size_t *nread);
int __NON_INSTRUMENT_FUNCTION__
zrt_zcall_prolog_pwrite(int fd, const void *buf, size_t count, off_t offset,
			    size_t *nwrote);
int zrt_zcall_prolog_seek(int fd, off_t offset, int whence, off_t *new_offset);
int zrt_zcall_prolog_fstat(int fd, struct stat *);
int zrt_zcall_prolog_getdents(int fd, struct dirent *, size_t count, size_t *nread);
/* irt filename *************************/
int zrt_zcall_prolog_open(const char *pathname, int oflag, mode_t cmode, int *newfd);
int zrt_zcall_prolog_stat(const char *pathname, struct stat *);
/* irt memory *************************/
int zrt_zcall_prolog_sysbrk(void **newbrk);
int zrt_zcall_prolog_mmap(void **addr, size_t len, int prot, int flags, int fd, off_t off);
int zrt_zcall_prolog_munmap(void *addr, size_t len);
/* irt tls *************************/
int zrt_zcall_prolog_tls_init(void *thread_ptr);
void *zrt_zcall_prolog_tls_get(void);
/* irt clock *************************/
int zrt_zcall_prolog_getres(clockid_t clk_id, struct timespec *res);
int zrt_zcall_prolog_gettime(clockid_t clk_id, struct timespec *tp);

/************************** zcalls_zrt_t functions **************/
void zrt_zcall_prolog_zrt_setup(void);
void zrt_zcall_prolog_premain(void);
void zrt_zcall_prolog_postmain(int exitcode);

/************************** zcalls_prolog_t functions **************/
void zrt_zcall_prolog_read_nvram_args_envs(int *arg_array_lengths, int *arg_count,
					   int *env_array_lengths, int *env_count);
void zrt_zcall_prolog_get_nvram_args_envs(char** args, char** envs);

/************************** zcalls_env_args_init_t functions **************/
void zrt_zcall_prolog_nvram_read_get_args_envs( int *args_buf_size, 
						int *envs_buf_size, int *env_count );
void zrt_zcall_prolog_nvram_get_args_envs( char** args, char* args_buf, int args_buf_size,
					   char** envs, char* envs_buf, int envs_buf_size );


/***************************************************************************
* Declaration of ZLIBC syscall implementations that used as secondary syscall
* handlers with enhanced functionality, can be used only after prolog init done;
***************************************************************************/

/* irt basic *************************/
void zrt_zcall_enhanced_exit(int status);
/* irt fdio *************************/
int zrt_zcall_enhanced_close(int fd);
int zrt_zcall_enhanced_dup(int fd);
int zrt_zcall_enhanced_dup2(int fd, int newfd);
int __NON_INSTRUMENT_FUNCTION__
zrt_zcall_enhanced_read(int fd, void *buf, size_t count, size_t *nread);
int __NON_INSTRUMENT_FUNCTION__
zrt_zcall_enhanced_write(int fd, const void *buf, size_t count, size_t *nwrote);
int __NON_INSTRUMENT_FUNCTION__
zrt_zcall_enhanced_pread(int fd, void *buf, size_t count, off_t offset, 
			     size_t *nread);
int __NON_INSTRUMENT_FUNCTION__
zrt_zcall_enhanced_pwrite(int fd, const void *buf, size_t count, off_t offset,
			      size_t *nwrote);
int zrt_zcall_enhanced_seek(int fd, off_t offset, int whence, off_t *new_offset);
int zrt_zcall_enhanced_fstat(int fd, struct stat *);
int zrt_zcall_enhanced_getdents(int fd, struct dirent *, size_t count, size_t *nread);
/* irt filename *************************/
int zrt_zcall_enhanced_open(const char *pathname, int oflag, mode_t cmode, int *newfd);
int zrt_zcall_enhanced_stat(const char *pathname, struct stat *);
/* irt memory *************************/
int zrt_zcall_enhanced_sysbrk(void **newbrk);
int zrt_zcall_enhanced_mmap(void **addr, size_t len, int prot, int flags, int fd, off_t off);
int zrt_zcall_enhanced_munmap(void *addr, size_t len);

/************************** zcalls_zrt_t functions **************/
void zrt_zcall_enhanced_zrt_setup(void);
void zrt_zcall_enhanced_premain(void);
void zrt_zcall_enhanced_postmain(int exitcode);

/*nonsyscalls*/
int zrt_zcall_fcntl(int fd, int cmd, ...);
int zrt_zcall_rename (const char *old, const char *new);
ssize_t zrt_zcall_readlink(const char *path, char *buf, size_t bufsize);
int zrt_zcall_symlink(const char *oldpath, const char *newpath);
int zrt_zcall_statvfs(const char* path, struct statvfs *buf);
int zrt_zcall_link(const char *oldpath, const char *newpath);
int zrt_zcall_unlink(const char *pathname);
int zrt_zcall_rmdir(const char *pathdir);
/*Create directory
 *@param mode umasked mode*/
int zrt_zcall_mkdir(const char *pathdir, mode_t mode);
int zrt_zcall_chmod(const char *path, mode_t mode);
int zrt_zcall_fchmod(int fd, mode_t mode);
int zrt_zcall_chown(const char *path, uid_t owner, gid_t group);
int zrt_zcall_fchown(int fd, uid_t owner, gid_t group);
int zrt_zcall_ftruncate(int fd, off_t length);
int zrt_zcall_stat_realpath(const char *abspathname, struct stat *stat);
int zrt_zcall_get_phys_pages(void);
int zrt_zcall_get_avphys_pages(void);
int zrt_zcall_fchdir(int fd);

#endif //__ZCALLS_H__

