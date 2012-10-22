/*
 * syscallbacks.c
 * Syscallbacks implementation used by zrt;
 *
 *  Created on: 6.07.2012
 *      Author: YaroslavLitvinov
 */

#define _GNU_SOURCE

#include <time.h>
#include <sys/time.h>
#include <sys/types.h> //off_t
#include <sys/stat.h>
#include <string.h> //memcpy
#include <stdio.h>
#include <stdlib.h> //atoi
#include <stddef.h> //size_t
#include <unistd.h> //STDIN_FILENO
#include <fcntl.h> //file flags, O_ACCMODE
#include <errno.h>
#include <dirent.h>     /* Defines DT_* constants */
#include <assert.h>

#include "zvm.h"
#include "zrt.h"
#include "zrtsyscalls.h"
#include "zrt_helper_macros.h"
#include "zrtlog.h"
#include "transparent_mount.h"
#include "stream_reader.h"
#include "unpack_tar.h" //tar unpacker
#include "mounts_manager.h"
#include "mem_mount_wraper.h"
#include "nacl_struct.h"
#include "image_engine.h"
#include "channels_mount.h"


/* TODO
 * RESEARCH
 * #include <mcheck.h> research debugging capabilities
 * rename : ENOSYS 38 Function not implemented
 * channels close, reopen, reclose
 * BUGS
 * fdopen failed, ftell fread
 * */

/*If it's enabled then zrt will try to load tarball from /dev/tarball channel*/
#define TARBALLFS

#ifdef DEBUG
#define ZRT_LOG_NAME "/dev/debug"
#endif //DEBUG



/****************** static data*/
struct timeval s_cached_timeval;
const struct UserManifest* s_manifest=NULL;
struct MountsInterface* s_channels_mount=NULL;
struct MountsInterface* s_mem_mount=NULL;
static struct MountsManager* s_mounts_manager;
static struct MountsInterface* s_transparent_mount;
/****************** */

/*return 0 if not valid, 1 if valid*/
static int validate_pointer_range(const void* ptr){
    if ( s_manifest && ptr ){
        if ( ptr < s_manifest->heap_ptr ){
            return 0;
        }
        else
            return 1;
    }else
        return 0;
}

static inline void update_cached_time()
{
    /* update time value
     * update seconds because updating miliseconds has no effect*/
    ++s_cached_timeval.tv_sec;
}

/* defined in nacl_struct.h
 * copy stat to nacl stat, strustures has vriable sizes*/
void set_nacl_stat( const struct stat* stat, struct nacl_abi_stat* nacl_stat ){
    nacl_stat->nacl_abi_st_dev = stat->st_dev;
    nacl_stat->nacl_abi_st_ino = stat->st_ino;
    nacl_stat->nacl_abi_st_mode = stat->st_mode;
    nacl_stat->nacl_abi_st_nlink = stat->st_nlink;
    nacl_stat->nacl_abi_st_uid = stat->st_uid;
    nacl_stat->nacl_abi_st_gid = stat->st_gid;
    nacl_stat->nacl_abi_st_rdev = stat->st_rdev;
    nacl_stat->nacl_abi_st_size = stat->st_size;
    nacl_stat->nacl_abi_st_blksize = stat->st_blksize;
    nacl_stat->nacl_abi_st_blocks = stat->st_blocks;
    nacl_stat->nacl_abi_st_atime = stat->st_atime;
    nacl_stat->nacl_abi_st_atimensec = 0;
    nacl_stat->nacl_abi_st_mtime = stat->st_mtime;
    nacl_stat->nacl_abi_st_mtimensec = 0;
    nacl_stat->nacl_abi_st_ctime = stat->st_ctime;
    nacl_stat->nacl_abi_st_ctimensec = 0;
}

static void debug_mes_open_flags( int flags )
{
    int all_flags[] = {O_CREAT, O_EXCL, O_TRUNC, O_DIRECT, O_DIRECTORY,
		       O_NOATIME, O_APPEND, O_ASYNC, O_SYNC, O_NONBLOCK, O_NDELAY, O_NOCTTY};
    char *all_texts[] = {"O_CREAT", "O_EXCL", "O_TRUNC", "O_DIRECT", "O_DIRECTORY",
			 "O_NOATIME", "O_APPEND", "O_ASYNC", "O_SYNC", "O_NONBLOCK", "O_NDELAY", "O_NOCTTY"};
    int i;
    assert( sizeof(all_flags)/sizeof(int) == sizeof(all_texts)/sizeof(char*) );

    for ( i=0; i < sizeof(all_flags)/sizeof(int); i++ ){
        if ( CHECK_FLAG(flags, all_flags[i]) ){
            zrt_log("flag %s=%d", all_texts[i], CHECK_FLAG(flags, all_flags[i]) );
        }
    }
}

static void debug_mes_stat(struct stat *stat){
    zrt_log("st_dev=%lld", stat->st_dev);
    zrt_log("st_ino=%lld", stat->st_ino);
    zrt_log("nlink=%d", stat->st_nlink);
    zrt_log("st_mode=%o(octal)", stat->st_mode);
    zrt_log("st_blksize=%d", (int)stat->st_blksize);
    zrt_log("st_size=%lld", stat->st_size);
    zrt_log("st_blocks=%d", (int)stat->st_blocks);
    zrt_log("st_atime=%lld", stat->st_atime );
    zrt_log("st_mtime=%lld", stat->st_mtime );
}


/*
 * alloc absolute path, for relative path just insert into beginning '/' char, 
 * for absolute path just alloc and return. user application can provide relative path, 
 * currently any of zrt filesystems does not supported relative path, so making absolute 
 * path is required.
 */
static char* alloc_absolute_path_from_relative( const char* path )
{
    /* some applications providing relative path, currently any of zrt filesystems 
     * does not support relative path, so make absolute path just insert '/' into
     * begin of relative path */
    char* absolute_path = malloc( strlen(path) + 2 );
    /*transform . path into root /  */
    if ( strlen(path) == 1 && path[0] == '.' ){
        strcpy( absolute_path, "/\0" );
    }
    /*transform ./ path into root / */
    else if ( strlen(path) == 2 && path[0] == '.' && path[1] == '/' ){
        strcpy( absolute_path, "/\0" );
    }
    /*if relative path is detected then transform it to absolute*/
    else if ( strlen(path) > 1 && path[0] != '/' ){
        strcpy( absolute_path, "/\0" );
        strcat(absolute_path, path);
    }
    else{
        strcpy(absolute_path, path);
    }
    return absolute_path;
}

static mode_t get_umask(){
    mode_t prev_umask=0;
    const char* prev_umask_str = getenv(UMASK_ENV);
    if ( prev_umask_str ){
        sscanf( prev_umask_str, "%o", &prev_umask);
    }
    return prev_umask;
}

static mode_t apply_umask(mode_t mode){
    mode_t umasked_mode = ~get_umask() & mode; /*reset umask bits for specified mode*/
    zrt_log( "mode=%o, umasked mode=%o", mode, umasked_mode );
    return umasked_mode;
}


/*************************************************************************
 * glibc substitution. Implemented functions below should be linked
 * instead of standard syscall that not implemented by NACL glibc
 **************************************************************************/

int mkdir(const char* pathname, mode_t mode){
    LOG_SYSCALL_START(NULL);
    errno=0;
    zrt_log("pathname=%p, mode=%o(octal), (pathname==0)=%d, (pathname==NULL)=%d, %X", pathname,
            (uint32_t)mode, (pathname==0), (pathname==NULL), (uintptr_t)pathname);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    char* absolute_path = alloc_absolute_path_from_relative( pathname );
    mode = apply_umask(mode);
    int ret = s_transparent_mount->mkdir( absolute_path, mode );
    int errno_mkdir = errno; /*save mkdir errno before stat request*/
    /*print stat data of newly created directory*/
    struct stat st;
    int ret2 = s_transparent_mount->stat(absolute_path, &st);
    if ( ret2 == 0 ){
        debug_mes_stat(&st);
    }
    /**/
    free(absolute_path);
    errno = errno_mkdir;/*restore mkdir errno after stat request completed*/
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/*glibc substitution. it should be linked instead standard rmdir */
int rmdir(const char *pathname){
    LOG_SYSCALL_START(NULL);
    errno=0;
    zrt_log("pathname=%s", pathname);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    char* absolute_path = alloc_absolute_path_from_relative( pathname );
    int ret = s_transparent_mount->rmdir( absolute_path );
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int lstat(const char *path, struct stat *buf){
    LOG_SYSCALL_START(NULL);
    errno=0;
    zrt_log("path=%s, buf=%p", path, buf);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    char* absolute_path = alloc_absolute_path_from_relative( path );
    int ret = s_transparent_mount->stat(absolute_path, buf);
    free(absolute_path);
    if ( ret == 0 ){
        debug_mes_stat(buf);
    }
    LOG_SYSCALL_FINISH(ret);
    return ret;
}


/*sets umask ang et previous value*/
mode_t umask(mode_t mask){
    LOG_SYSCALL_START(NULL);
    /*save new umask and return prev*/
    mode_t prev_umask = get_umask();
    char umask_str[11];
    sprintf( umask_str, "%o", mask );
    setenv( UMASK_ENV, umask_str, 1 );
    zrt_log("%s", umask_str);
    LOG_SYSCALL_FINISH(0);
    return prev_umask;
}

int chown(const char *path, uid_t owner, gid_t group){
    LOG_SYSCALL_START(NULL);
    errno=0;
    zrt_log( "path=%s, owner=%u, group=%u", path, owner, group );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    char* absolute_path = alloc_absolute_path_from_relative(path);
    int ret = s_transparent_mount->chown(absolute_path, owner, group);
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int fchown(int fd, uid_t owner, gid_t group){
    LOG_SYSCALL_START(NULL);
    errno=0;
    zrt_log( "fd=%d, owner=%u, group=%u", fd, owner, group );
    int ret = s_transparent_mount->fchown(fd, owner, group);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int lchown(const char *path, uid_t owner, gid_t group){
    LOG_SYSCALL_START(NULL);
    zrt_log( "path=%s, owner=%u, group=%u", path, owner, group );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    /*do not do transformaton path, it's called in nested chown*/
    int ret =chown(path, owner, group);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int unlink(const char *pathname){
    LOG_SYSCALL_START(NULL);
    errno=0;
    zrt_log( "pathname=%s", pathname );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    char* absolute_path = alloc_absolute_path_from_relative(pathname);
    int ret = s_transparent_mount->unlink(absolute_path);
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int chmod(const char *path, mode_t mode){
    LOG_SYSCALL_START(NULL);
    errno=0;
    zrt_log( "path=%s, mode=%u", path, mode );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    mode = apply_umask(mode);
    char* absolute_path = alloc_absolute_path_from_relative(path);
    int ret = s_transparent_mount->chmod(absolute_path, mode);
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int fchmod(int fd, mode_t mode){
    LOG_SYSCALL_START(NULL);
    errno=0;
    zrt_log( "fd=%d, mode=%u", fd, mode );
    mode = apply_umask(mode);
    int ret = s_transparent_mount->fchmod(fd, mode);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/*override system glibc implementation due to bad errno at errors*/
int fseek(FILE *stream, long offset, int whence){
    LOG_SYSCALL_START(NULL);
    errno = 0;
    int handle = fileno(stream);
    int ret = s_transparent_mount->lseek(handle, offset, whence);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/********************************************************************************
 * ZRT IMPLEMENTATION OF NACL SYSCALLS
 * each nacl syscall must be implemented or, at least, mocked. no exclusions!
 *********************************************************************************/


SYSCALL_MOCK(null, 0)
SYSCALL_MOCK(nameservice, 0)
SYSCALL_MOCK(dup, -EPERM) /* duplicate the given file handle. n/a in the simple zrt version */
SYSCALL_MOCK(dup2, -EPERM) /* duplicate the given file handle. n/a in the simple zrt version */


/*
 * return channel handle by given name. if given flags
 * doesn't answer channel's type/limits
 */
static int32_t zrt_open(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    errno=0;
    char* name = (char*)args[0];
    int flags = (int)args[1];
    uint32_t mode = (int)args[2];

    zrt_log("path=%s", name);
    VALIDATE_SYSCALL_PTR(name);
    debug_mes_open_flags(flags);

    char* absolute_path = alloc_absolute_path_from_relative( name );
    mode = apply_umask(mode);
    int ret = s_transparent_mount->open( absolute_path, flags, mode );
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}


/* do nothing but checks given handle */
static int32_t zrt_close(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    errno = 0;
    int handle = (int)args[0];

    int ret = s_transparent_mount->close(handle);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}


/* read the file with the given handle number */
static int32_t zrt_read(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    errno = 0;
    int handle = (int)args[0];
    void *buf = (void*)args[1];
    VALIDATE_SYSCALL_PTR(buf);
    int64_t length = (int64_t)args[2];

    int32_t ret = s_transparent_mount->read(handle, buf, length);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/* example how to implement zrt syscall */
static int32_t zrt_write(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    zrt_log("errno=%d", errno);
    int handle = (int)args[0];
    void *buf = (void*)args[1];
    VALIDATE_SYSCALL_PTR(buf);
    int64_t length = (int64_t)args[2];

#ifdef DEBUG
    /*disable logging write calls related to debug, stdout and stderr channel */
    if ( handle <= 2 || handle == zrt_log_fd() ){
        disable_logging_current_syscall();
    }
#endif


    int32_t ret = s_transparent_mount->write(handle, buf, length);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/*
 * seek for the new zerovm channels design
 */
static int32_t zrt_lseek(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    errno = 0;
    int32_t handle = (int32_t)args[0];
    off_t offset = *((off_t*)args[1]);
    int whence = (int)args[2];

    offset = s_transparent_mount->lseek(handle, offset, whence);
    *(off_t *)args[1] = offset;
    LOG_SYSCALL_FINISH(offset);
    return offset;
}


SYSCALL_MOCK(ioctl, -EINVAL) /* not implemented in the simple version of zrtlib */


/*
 * return synthetic channel information
 * todo(d'b): the function needs update after the channels design will complete
 */
static int32_t zrt_stat(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    errno = 0;
    const char *file = (const char*)args[0];
    struct nacl_abi_stat *sbuf = (struct nacl_abi_stat *)args[1];
    VALIDATE_SYSCALL_PTR(file);
    VALIDATE_SYSCALL_PTR(sbuf);
    struct stat st;
    char* absolute_path = alloc_absolute_path_from_relative(file);
    int ret = s_transparent_mount->stat(absolute_path, &st);
    free(absolute_path);
    if ( ret == 0 ){
        debug_mes_stat(&st);
        set_nacl_stat( &st, sbuf ); //convert from nacl_stat into stat
    }
    LOG_SYSCALL_FINISH(ret);
    return ret;
}


/* return synthetic channel information */
static int32_t zrt_fstat(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    errno = 0;
    int handle = (int)args[0];
    struct nacl_abi_stat *sbuf = (struct nacl_abi_stat *)args[1];
    VALIDATE_SYSCALL_PTR(sbuf);
    struct stat st;
    int ret = s_transparent_mount->fstat( handle, &st);
    if ( ret == 0 ){
        debug_mes_stat(&st);
        set_nacl_stat( &st, sbuf ); //convert from nacl_stat into stat
    }
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

SYSCALL_MOCK(chmod, -EPERM) /* NACL does not support chmod*/

/*
 * following 3 functions (sysbrk, mmap, munmap) is the part of the
 * new memory engine. the new allocate specified amount of ram before
 * user code start and then user can only obtain that allocated memory.
 * zrt lib will help user to do it transparently.
 */

/* change space allocation. ZRT nothing do here just call sysbrk NACL syscall.*/
static int32_t zrt_sysbrk(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    int32_t retcode;

    zvm_syscallback(0); /* uninstall syscallback */
    retcode = NaCl_sysbrk(args[0]); /* invoke syscall directly */
    zvm_syscallback((intptr_t)syscall_director); /* reinstall syscallback */

    LOG_SYSCALL_FINISH(retcode);
    return retcode;
}

/* map region of memory. ZRT nothing do here just call mmap NACL syscall.*/
static int32_t zrt_mmap(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    int32_t retcode;

    zvm_syscallback(0); /* uninstall syscallback */
    retcode = NaCl_mmap(args[0], args[1], args[2], args[3], args[4], args[5]);
    zvm_syscallback((intptr_t)syscall_director); /* reinstall syscallback */

    LOG_SYSCALL_FINISH(retcode);
    return retcode;
}

/*
 * unmap region of memory
 * note: zerovm doesn't use it in memory management.
 * instead of munmap it use mmap with protection 0
 * ZRT nothing do here just call  munmap NACL syscall.*/
static int32_t zrt_munmap(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    int32_t retcode;

    zvm_syscallback(0); /* uninstall syscallback */
    retcode = NaCl_munmap(args[0], args[1]);
    zvm_syscallback((intptr_t)syscall_director); /* reinstall syscallback */

    LOG_SYSCALL_FINISH(retcode);
    return retcode;
}


static int32_t zrt_getdents(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    errno=0;
    int handle = (int)args[0];
    char *buf = (char*)args[1];
    VALIDATE_SYSCALL_PTR(buf);
    uint32_t count = args[2];

    int32_t ret = s_transparent_mount->getdents(handle, buf, count);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}


/*
 * exit. most important syscall. without it the user program
 * cannot terminate correctly.
 */
static int32_t zrt_exit(uint32_t *args)
{
    /* no need to check args for NULL. it is always set by syscall_manager */
    LOG_SYSCALL_START(args);
    zvm_exit(args[0]);
    LOG_SYSCALL_FINISH(0);
    return 0; /* unreachable */
}

SYSCALL_MOCK(getpid, 0)
SYSCALL_MOCK(sched_yield, 0)
SYSCALL_MOCK(sysconf, 0)

/* if given in manifest let user to have it */
#define TIMESTAMP_STR "TimeStamp"
static int32_t zrt_gettimeofday(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    struct nacl_abi_timeval  *tv = (struct nacl_abi_timeval *)args[0];
    int ret=0;
    errno=0;

    if(!tv) {
        errno = EFAULT;
        ret =-1;
    }
    else{
        /*retrieve and get cached time value*/
        tv->nacl_abi_tv_usec = s_cached_timeval.tv_usec;
        tv->nacl_abi_tv_sec = s_cached_timeval.tv_sec;
        zrt_log("tv->nacl_abi_tv_sec=%lld, tv->nacl_abi_tv_usec=%d", tv->nacl_abi_tv_sec, tv->nacl_abi_tv_usec );

        /* update time value*/
        update_cached_time();
    }

    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/*
 * should never be implemented if we want deterministic behaviour
 * note: but we can allow to return each time synthetic value
 * warning! after checking i found that nacl is not using it, so this
 *          function is useless for current nacl sdk version.
 */
SYSCALL_MOCK(clock, -EPERM)
SYSCALL_MOCK(nanosleep, 0)
SYSCALL_MOCK(imc_makeboundsock, 0)
SYSCALL_MOCK(imc_accept, 0)
SYSCALL_MOCK(imc_connect, 0)
SYSCALL_MOCK(imc_sendmsg, 0)
SYSCALL_MOCK(imc_recvmsg, 0)
SYSCALL_MOCK(imc_mem_obj_create, 0)
SYSCALL_MOCK(imc_socketpair, 0)
SYSCALL_MOCK(mutex_create, 0)
SYSCALL_MOCK(mutex_lock, 0)
SYSCALL_MOCK(mutex_trylock, 0)
SYSCALL_MOCK(mutex_unlock, 0)
SYSCALL_MOCK(cond_create, 0)
SYSCALL_MOCK(cond_wait, 0)
SYSCALL_MOCK(cond_signal, 0)
SYSCALL_MOCK(cond_broadcast, 0)
SYSCALL_MOCK(cond_timed_wait_abs, 0)
SYSCALL_MOCK(thread_create, 0)
SYSCALL_MOCK(thread_exit, 0)
SYSCALL_MOCK(tls_init, 0)
SYSCALL_MOCK(thread_nice, 0)

/*
 * get tls from zerovm. can be replaced with untrusted version
 * after "blob library" loader will be ready
 */
static int32_t zrt_tls_get(uint32_t *args)
{
    /* switch off spam
     * LOG_SYSCALL_START(args);*/
    int32_t retcode;

    zvm_syscallback(0); /* uninstall syscallback */
    retcode = NaCl_tls_get(); /* invoke syscall directly */
    zvm_syscallback((intptr_t)syscall_director); /* reinstall syscallback */

    return retcode;
}

SYSCALL_MOCK(second_tls_set, 0)

/*
 * get second tls.
 * since we only have single thread use instead of 2nd tls 1st one
 */
static int32_t zrt_second_tls_get(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    int32_t ret = zrt_tls_get(NULL);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

SYSCALL_MOCK(sem_create, 0)
SYSCALL_MOCK(sem_wait, 0)
SYSCALL_MOCK(sem_post, 0)
SYSCALL_MOCK(sem_get_value, 0)
SYSCALL_MOCK(dyncode_create, 0)
SYSCALL_MOCK(dyncode_modify, 0)
SYSCALL_MOCK(dyncode_delete, 0)
SYSCALL_MOCK(test_infoleak, 0)

/* Every SYSCALL_NOT_IMPLEMENTED should have appropriate SYSCALL_STUB_IMPLEMENTATION 
 * to be correctly referenced inside of zrt_syscalls list*/
SYSCALL_STUB_IMPLEMENTATION(0)
SYSCALL_STUB_IMPLEMENTATION(3)
SYSCALL_STUB_IMPLEMENTATION(4)
SYSCALL_STUB_IMPLEMENTATION(5)
SYSCALL_STUB_IMPLEMENTATION(6)
SYSCALL_STUB_IMPLEMENTATION(7)
SYSCALL_STUB_IMPLEMENTATION(19)
SYSCALL_STUB_IMPLEMENTATION(24)
SYSCALL_STUB_IMPLEMENTATION(25)
SYSCALL_STUB_IMPLEMENTATION(26)
SYSCALL_STUB_IMPLEMENTATION(27)
SYSCALL_STUB_IMPLEMENTATION(28)
SYSCALL_STUB_IMPLEMENTATION(29)
SYSCALL_STUB_IMPLEMENTATION(34)
SYSCALL_STUB_IMPLEMENTATION(35)
SYSCALL_STUB_IMPLEMENTATION(36)
SYSCALL_STUB_IMPLEMENTATION(37)
SYSCALL_STUB_IMPLEMENTATION(38)
SYSCALL_STUB_IMPLEMENTATION(39)
SYSCALL_STUB_IMPLEMENTATION(43)
SYSCALL_STUB_IMPLEMENTATION(44)
SYSCALL_STUB_IMPLEMENTATION(45)
SYSCALL_STUB_IMPLEMENTATION(46)
SYSCALL_STUB_IMPLEMENTATION(47)
SYSCALL_STUB_IMPLEMENTATION(48)
SYSCALL_STUB_IMPLEMENTATION(49)
SYSCALL_STUB_IMPLEMENTATION(50)
SYSCALL_STUB_IMPLEMENTATION(51)
SYSCALL_STUB_IMPLEMENTATION(52)
SYSCALL_STUB_IMPLEMENTATION(53)
SYSCALL_STUB_IMPLEMENTATION(54)
SYSCALL_STUB_IMPLEMENTATION(55)
SYSCALL_STUB_IMPLEMENTATION(56)
SYSCALL_STUB_IMPLEMENTATION(57)
SYSCALL_STUB_IMPLEMENTATION(58)
SYSCALL_STUB_IMPLEMENTATION(59)
SYSCALL_STUB_IMPLEMENTATION(67)
SYSCALL_STUB_IMPLEMENTATION(68)
SYSCALL_STUB_IMPLEMENTATION(69)
SYSCALL_STUB_IMPLEMENTATION(78)
SYSCALL_STUB_IMPLEMENTATION(87)
SYSCALL_STUB_IMPLEMENTATION(88)
SYSCALL_STUB_IMPLEMENTATION(89)
SYSCALL_STUB_IMPLEMENTATION(90)
SYSCALL_STUB_IMPLEMENTATION(91)
SYSCALL_STUB_IMPLEMENTATION(92)
SYSCALL_STUB_IMPLEMENTATION(93)
SYSCALL_STUB_IMPLEMENTATION(94)
SYSCALL_STUB_IMPLEMENTATION(95)
SYSCALL_STUB_IMPLEMENTATION(96)
SYSCALL_STUB_IMPLEMENTATION(97)
SYSCALL_STUB_IMPLEMENTATION(98)
SYSCALL_STUB_IMPLEMENTATION(99)
SYSCALL_STUB_IMPLEMENTATION(107)
SYSCALL_STUB_IMPLEMENTATION(108)



/*
 * array of the pointers to zrt syscalls
 *
 * each zrt function (syscall) has uniform prototype: int32_t syscall(uint32_t *args)
 * where "args" pointer to syscalls' arguments. number and types of arguments
 * can be found in the nacl "nacl_syscall_handlers.c" file.
 * 
 * SYSCALL_NOT_IMPLEMENTED would be used for unsupported syscalls, any that definition
 * should have appropriate SYSCALL_STUB_IMPLEMENTATION to be correctly referenced
 */
int32_t (*zrt_syscalls[])(uint32_t*) = {
    SYSCALL_NOT_IMPLEMENTED(0),/* 0 -- not implemented syscall */
    zrt_null,                  /* 1 -- empty syscall. does nothing */
    zrt_nameservice,           /* 2 */
    SYSCALL_NOT_IMPLEMENTED(3),/* 3 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(4),/* 4 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(5),/* 5 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(6),/* 6 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(7),/* 7 -- not implemented syscall */
    zrt_dup,                   /* 8 */
    zrt_dup2,                  /* 9 */
    zrt_open,                  /* 10 */
    zrt_close,                 /* 11 */
    zrt_read,                  /* 12 */
    zrt_write,                 /* 13 */
    zrt_lseek,                 /* 14 */
    zrt_ioctl,                 /* 15 */
    zrt_stat,                  /* 16 */
    zrt_fstat,                 /* 17 */
    zrt_chmod,                 /* 18 */
    SYSCALL_NOT_IMPLEMENTED(19),/* 19 -- not implemented syscall */
    zrt_sysbrk,                /* 20 */
    zrt_mmap,                  /* 21 */
    zrt_munmap,                /* 22 */
    zrt_getdents,              /* 23 */
    SYSCALL_NOT_IMPLEMENTED(24),/* 24 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(25),/* 25 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(26),/* 26 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(27),/* 27 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(28),/* 28 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(29),/* 29 -- not implemented syscall */
    zrt_exit,                  /* 30 -- must use trap:exit() */
    zrt_getpid,                /* 31 */
    zrt_sched_yield,           /* 32 */
    zrt_sysconf,               /* 33 */
    SYSCALL_NOT_IMPLEMENTED(34),/* 34 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(35),/* 35 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(36),/* 36 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(37),/* 37 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(38),/* 38 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(39),/* 39 -- not implemented syscall */
    zrt_gettimeofday,          /* 40 */
    zrt_clock,                 /* 41 */
    zrt_nanosleep,             /* 42 */
    SYSCALL_NOT_IMPLEMENTED(43),/* 43 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(44),/* 44 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(45),/* 45 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(46),/* 46 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(47),/* 47 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(48),/* 48 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(49),/* 49 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(50),/* 50 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(51),/* 51 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(52),/* 52 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(53),/* 53 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(54),/* 54 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(55),/* 55 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(56),/* 56 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(57),/* 57 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(58),/* 58 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(59),/* 59 -- not implemented syscall */
    zrt_imc_makeboundsock,     /* 60 */
    zrt_imc_accept,            /* 61 */
    zrt_imc_connect,           /* 62 */
    zrt_imc_sendmsg,           /* 63 */
    zrt_imc_recvmsg,           /* 64 */
    zrt_imc_mem_obj_create,    /* 65 */
    zrt_imc_socketpair,        /* 66 */
    SYSCALL_NOT_IMPLEMENTED(67),/* 67 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(68),/* 68 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(69),/* 69 -- not implemented syscall */
    zrt_mutex_create,          /* 70 */
    zrt_mutex_lock,            /* 71 */
    zrt_mutex_trylock,         /* 72 */
    zrt_mutex_unlock,          /* 73 */
    zrt_cond_create,           /* 74 */
    zrt_cond_wait,             /* 75 */
    zrt_cond_signal,           /* 76 */
    zrt_cond_broadcast,        /* 77 */
    SYSCALL_NOT_IMPLEMENTED(78),/* 78 -- not implemented syscall */
    zrt_cond_timed_wait_abs,   /* 79 */
    zrt_thread_create,         /* 80 */
    zrt_thread_exit,           /* 81 */
    zrt_tls_init,              /* 82 */
    zrt_thread_nice,           /* 83 */
    zrt_tls_get,               /* 84 */
    zrt_second_tls_set,        /* 85 */
    zrt_second_tls_get,        /* 86 */
    SYSCALL_NOT_IMPLEMENTED(87),/* 87 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(88),/* 88 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(89),/* 89 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(90),/* 90 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(91),/* 91 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(92),/* 92 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(93),/* 93 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(94),/* 94 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(95),/* 95 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(96),/* 96 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(97),/* 97 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(98),/* 98 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(99),/* 99 -- not implemented syscall */
    zrt_sem_create,            /* 100 */
    zrt_sem_wait,              /* 101 */
    zrt_sem_post,              /* 102 */
    zrt_sem_get_value,         /* 103 */
    zrt_dyncode_create,        /* 104 */
    zrt_dyncode_modify,        /* 105 */
    zrt_dyncode_delete,        /* 106 */
    SYSCALL_NOT_IMPLEMENTED(107),/* 107 -- not implemented syscall */
    SYSCALL_NOT_IMPLEMENTED(108),/* 108 -- not implemented syscall */
    zrt_test_infoleak          /* 109 */
};


void zrt_setup( struct UserManifest* manifest ){
    /*save UserManifest data for ZRT purposes to avoid request it via ZVM api repeatedly*/
    s_manifest = manifest;
    /*manage mounted filesystems*/
    s_mounts_manager = alloc_mounts_manager();
    /*alloc filesystem based on channels*/
    s_channels_mount = alloc_channels_mount( s_mounts_manager->handle_allocator,
					     manifest->channels, manifest->channels_count );
    /*alloc entry point to mounted filesystems*/
    s_transparent_mount = alloc_transparent_mount( s_mounts_manager );

    /* using direct call to channels_mount and create debuging log channel*/
    int zrt_log_fd = s_channels_mount->open( ZRT_LOG_NAME, O_WRONLY, 0 ); /*open log channel*/
    set_zrtlog_fd( zrt_log_fd );

    zrt_log_str( "started" );
}

void zrt_setup_finally(){
    /* using of channels_mount directly to preallocate standard channels sdtin, stdout, stderr */
    s_channels_mount->open( DEV_STDIN, O_RDONLY, 0 );
    s_channels_mount->open( DEV_STDOUT, O_WRONLY, 0 );
    s_channels_mount->open( DEV_STDERR, O_WRONLY, 0 );

    /* get time stamp from the environment, and cache it */
    char *stamp = getenv( TIMESTAMP_STR );
    if ( stamp && *stamp ){
        s_cached_timeval.tv_usec = 0; /* msec not supported by nacl */
        s_cached_timeval.tv_sec = atoi(stamp); /* manifest always contain decimal values */
        zrt_log("s_cached_timeval.nacl_abi_tv_sec=%lld", s_cached_timeval.tv_sec );
    }

    /*create mem mount*/
    s_mem_mount = alloc_mem_mount( s_mounts_manager->handle_allocator );

    /*Mount channels filesystem as root*/
    s_mounts_manager->mount_add( "/dev", s_channels_mount );
    s_mounts_manager->mount_add( "/", s_mem_mount );

#ifdef TARBALLFS
    /*
     * *load filesystem from cdr channel. Content of ilesystem is reading from cdr channel that points
     * to supported archive, read every file and add it contents into MemMount filesystem
     * */
    /*create stream reader linked to tar archive that contains filesystem image*/
    struct StreamReader* stream_reader = alloc_stream_reader( s_channels_mount, DEV_IMAGE );

    if ( stream_reader ){
        /*create image loader, passed 1st param: image alias, 2nd param: Root filesystem;
         * Root filesystem passed instead MemMount to reject adding of files into /dev folder;
         * For example if archive contains non empty /dev folder, that contents will be ignored*/
        struct ImageInterface* image_loader = alloc_image_loader( s_transparent_mount );
        /*create archive unpacker, channels_mount is useing to read channel stream*/
        struct UnpackInterface* tar_unpacker = alloc_unpacker_tar( stream_reader, image_loader->observer_implementation );

        /*read archive from linked channel and add all contents into Filesystem*/
        int count_files = image_loader->deploy_image( "/", tar_unpacker );
        zrt_log( "Added %d files to MemFS", count_files );

        free_unpacker_tar( tar_unpacker );
        free_image_loader( image_loader );
        free_stream_reader( stream_reader );
    }
    ZRT_LOG_DELIMETER;
#endif
}

