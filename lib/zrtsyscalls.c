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
 * fstat / stat changes according to channel types
 * rename : ENOSYS 38 Function not implemented
 * CURRENTLY
 * fseek uses long offset
 * lseek uses off_t offset
 * ftell pos for std* streams
 * SUPPORT
 * fstat for /etc/localtime, set timestamp as localtime
 * big file size 5gb _FILE_OFFSET_BITS=64
 * use fpos64_t; fgetpos64, fsetpos64
 * * _FILE_OFFSET_BITS=32
 * use fpos_t; fgetpos, fsetpos
 * EOF all cases:
 * - zvm_pread return 0 bytes, errno 0;
 * - zvm_pread return err code, if limits reached;
 * - read - bytes read is returned (zero indicates end of file;
 * - pread - zero indicates that end of file
 * - test EOF for channel files: getchar, getline
 * - feof - returning nonzero if it is set to EOF
 * TESTS
 * : EOF correctly return by read, fread, feof,*
 * : channels close, reopen, reclose
 * : try to test fstat returned data
 * BUGS
 * : fdopen failed, ftell fread
 * */

/*If it's enabled then zrt will try to load tarball from /dev/tarball channel*/
#define TARBALLFS

/* ******************************************************************************
 * Syscallbacks debug macros*/
#ifdef DEBUG
#define ZRT_LOG_NAME "/dev/debug"
#define LOG_SYSCALL_START(args_p) { log_push_name(__func__); enable_logging_current_syscall(); zrt_log("syscall arg[0]=0x%x, arg[1]=0x%x, arg[2]=0x%x, arg[3]=0x%x, arg[4]=0x%x", \
        args_p[0], args_p[1], args_p[2], args_p[3], args_p[4] ); }
#define LOG_SYSCALL_FINISH(ret){ \
        if ( ret == -1 ){ zrt_log("ret=0x%x, errno=%d", (int)ret, errno);} \
        else{ zrt_log("ret=0x%x", (int)ret);} \
        log_pop_name(__func__); }
#define LOG_START() { log_push_name(__func__); enable_logging_current_syscall(); zrt_log("%s", __func__); }
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

#define SYSCALL_VALIDATE_POINTER(arg_pointer) \
        if ( arg_pointer == NULL ) \
        { errno=EFAULT; LOG_SYSCALL_FINISH(-1); return -1; }

#define BUGGY_SYSCALL_VALIDATE_POINTER(arg_pointer) { \
        char str[20]; sprintf(str, "%p", arg_pointer); \
        if ( arg_pointer == NULL || !strcmp(str, "(nil)") ) \
        { errno=EFAULT; LOG_SYSCALL_FINISH(-1); return -1; } }


/****************** static data*/
struct timeval s_cached_timeval;
const struct UserManifest* s_manifest=NULL;
struct MountsInterface* s_channels_mount=NULL;
struct MountsInterface* s_mem_mount=NULL;
static struct MountsManager* s_mounts_manager;
static struct MountsInterface* s_transparent_mount;
/****************** */

void update_cached_time()
{
    /* update time value
     * update seconds because updating miliseconds has no effect*/
    ++s_cached_timeval.tv_sec;
}

void set_zrt_errno( int zvm_errno )
{
    errno = zvm_errno;
    zrt_log( "errno=%d", errno );
}

/*copy stat to nacl stat, strustures has vriable sizes*/
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
    /*
     * O_CREAT,      cdr =
     * O_EXCL,       cdr
     * O_TRUNC      provided by trusted side
     * O_LARGEFILE,  native support should be
     * O_DIRECT              required _GNU_SOURCE define do we need it?
     * O_DIRECTORY,  cdr     required _GNU_SOURCE define
     * O_NOATIME, ignore     required _GNU_SOURCE define
     * O_NOFOLLOW, ignore    (absent in nacl headers)
     * O_APPEND support for cdr
     * O_CREAT  provided by trusted side
     * O_ASYNC
     * O_CLOEXEC ignore non portable
     * O_SYNC, ignore
     * O_NONBLOCK, O_NDELAY, O_NOCTTY,   does not supported
     * */
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

//static void get_tm_from_timeval(const struct timeval* tv, struct tm* tm){
//#define SECONDS_IN_YEAR
////    struct tm {
////         int tm_sec;         /* seconds */
////         int tm_min;         /* minutes */
////         int tm_hour;        /* hours */
////         int tm_mday;        /* day of the month */
////         int tm_mon;         /* month */
////         int tm_year;        /* year */
////         int tm_wday;        /* day of the week */
////         int tm_yday;        /* day in the year */
////         int tm_isdst;       /* daylight saving time */
////     };
//
//
//    tv->tv_sec
//}

//static void debug_log_permissions( mode_t mode ){
//    S_IFMT     0170000   bit mask for the file type bit fields
//    S_IFSOCK   0140000   socket
//    S_IFLNK    0120000   symbolic link
//    S_IFREG    0100000   regular file
//    S_IFBLK    0060000   block device
//    S_IFDIR    0040000   directory
//    S_IFCHR    0020000   character device
//    S_IFIFO    0010000   FIFO
//    S_ISUID    0004000   set UID bit
//    S_ISGID    0002000   set-group-ID bit (see below)
//    S_ISVTX    0001000   sticky bit (see below)
//    S_IRWXU    00700     mask for file owner permissions
//    S_IRUSR    00400     owner has read permission
//    S_IWUSR    00200     owner has write permission
//    S_IXUSR    00100     owner has execute permission
//    S_IRWXG    00070     mask for group permissions
//    S_IRGRP    00040     group has read permission
//    S_IWGRP    00020     group has write permission
//    S_IXGRP    00010     group has execute permission
//    S_IRWXO    00007     mask for permissions for others (not in group)
//    S_IROTH    00004     others have read permission
//    S_IWOTH    00002     others have write permission
//    S_IXOTH    00001     others have execute permission
//
//}

/*alloc absolute path, for relative path jut insert into beginning '/' char, for absolute path just alloc and return.
 *user application can provide relative path, currently any of zrt filesystems does not supported
 *relative path, so making absolute path is required.
 * */
static char* alloc_absolute_path_from_relative( const char* path )
{
    /*some applications providing relative path, currently any of zrt filesystems does not supported
     *relative path, so make absolute path just insert '/' into begin of relative path */
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
    LOG_START();
    errno=0;
    zrt_log("pathname=%p, mode=%o(octal), (pathname==0)=%d, (pathname==NULL)=%d, %X", pathname,
            (uint32_t)mode, (pathname==0), (pathname==NULL), (uintptr_t)pathname);
    BUGGY_SYSCALL_VALIDATE_POINTER(pathname);
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
    LOG_START();
    errno=0;
    zrt_log("pathname=%s", pathname);
    BUGGY_SYSCALL_VALIDATE_POINTER(pathname);
    char* absolute_path = alloc_absolute_path_from_relative( pathname );
    int ret = s_transparent_mount->rmdir( absolute_path );
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int lstat(const char *path, struct stat *buf){
    LOG_START();
    errno=0;
    zrt_log("path=%s, buf=%p", path, buf);
    BUGGY_SYSCALL_VALIDATE_POINTER(path);
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
    LOG_START();
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
    LOG_START();
    errno=0;
    zrt_log( "path=%s, owner=%u, group=%u", path, owner, group );
    BUGGY_SYSCALL_VALIDATE_POINTER(path);
    char* absolute_path = alloc_absolute_path_from_relative(path);
    int ret = s_transparent_mount->chown(absolute_path, owner, group);
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int fchown(int fd, uid_t owner, gid_t group){
    LOG_START();
    errno=0;
    zrt_log( "fd=%d, owner=%u, group=%u", fd, owner, group );
    int ret = s_transparent_mount->fchown(fd, owner, group);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int lchown(const char *path, uid_t owner, gid_t group){
    LOG_START();
    zrt_log( "path=%s, owner=%u, group=%u", path, owner, group );
    BUGGY_SYSCALL_VALIDATE_POINTER(path);
    /*do not do transformaton path, it's called in nested chown*/
    int ret =chown(path, owner, group);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int unlink(const char *pathname){
    LOG_START();
    errno=0;
    zrt_log( "pathname=%s", pathname );
    BUGGY_SYSCALL_VALIDATE_POINTER(pathname);
    char* absolute_path = alloc_absolute_path_from_relative(pathname);
    int ret = s_transparent_mount->unlink(absolute_path);
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int chmod(const char *path, mode_t mode){
    LOG_START();
    errno=0;
    zrt_log( "path=%s, mode=%u", path, mode );
    BUGGY_SYSCALL_VALIDATE_POINTER(path);
    mode = apply_umask(mode);
    char* absolute_path = alloc_absolute_path_from_relative(path);
    int ret = s_transparent_mount->chmod(absolute_path, mode);
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int fchmod(int fd, mode_t mode){
    LOG_START();
    errno=0;
    zrt_log( "fd=%d, mode=%u", fd, mode );
    mode = apply_umask(mode);
    int ret = s_transparent_mount->fchmod(fd, mode);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}



/********************************************************************************
 * ZRT IMPLEMENTATION OF NACL SYSCALLS
 * each nacl syscall must be implemented or, at least, mocked. no exclusions!
 *********************************************************************************/

static int32_t zrt_nan(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    LOG_SYSCALL_FINISH(0);
    return 0;
}
//SYSCALL_MOCK(nan, 0)

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
    SYSCALL_VALIDATE_POINTER(name);
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
    SYSCALL_VALIDATE_POINTER(buf);
    int64_t length = (int64_t)args[2];

    int32_t ret = s_transparent_mount->read(handle, buf, length);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/* example how to implement zrt syscall */
static int32_t zrt_write(uint32_t *args)
{
    LOG_SYSCALL_START(args);
    int handle = (int)args[0];
    void *buf = (void*)args[1];
    SYSCALL_VALIDATE_POINTER(buf);
    int64_t length = (int64_t)args[2];

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
    SYSCALL_VALIDATE_POINTER(file);
    SYSCALL_VALIDATE_POINTER(sbuf);
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
    SYSCALL_VALIDATE_POINTER(sbuf);
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

/* change space allocation */
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

/* map region of memory */
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
 */
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
    SYSCALL_VALIDATE_POINTER(buf);
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

/*
 * array of the pointers to zrt syscalls
 *
 * each zrt function (syscall) has uniform prototype: int32_t syscall(uint32_t *args)
 * where "args" pointer to syscalls' arguments. number and types of arguments
 * can be found in the nacl "nacl_syscall_handlers.c" file.
 */
int32_t (*zrt_syscalls[])(uint32_t*) = {
        zrt_nan,                   /* 0 -- not implemented syscall */
        zrt_null,                  /* 1 -- empty syscall. does nothing */
        zrt_nameservice,           /* 2 */
        zrt_nan,                   /* 3 -- not implemented syscall */
        zrt_nan,                   /* 4 -- not implemented syscall */
        zrt_nan,                   /* 5 -- not implemented syscall */
        zrt_nan,                   /* 6 -- not implemented syscall */
        zrt_nan,                   /* 7 -- not implemented syscall */
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
        zrt_nan,                   /* 19 -- not implemented syscall */
        zrt_sysbrk,                /* 20 */
        zrt_mmap,                  /* 21 */
        zrt_munmap,                /* 22 */
        zrt_getdents,              /* 23 */
        zrt_nan,                   /* 24 -- not implemented syscall */
        zrt_nan,                   /* 25 -- not implemented syscall */
        zrt_nan,                   /* 26 -- not implemented syscall */
        zrt_nan,                   /* 27 -- not implemented syscall */
        zrt_nan,                   /* 28 -- not implemented syscall */
        zrt_nan,                   /* 29 -- not implemented syscall */
        zrt_exit,                  /* 30 -- must use trap:exit() */
        zrt_getpid,                /* 31 */
        zrt_sched_yield,           /* 32 */
        zrt_sysconf,               /* 33 */
        zrt_nan,                   /* 34 -- not implemented syscall */
        zrt_nan,                   /* 35 -- not implemented syscall */
        zrt_nan,                   /* 36 -- not implemented syscall */
        zrt_nan,                   /* 37 -- not implemented syscall */
        zrt_nan,                   /* 38 -- not implemented syscall */
        zrt_nan,                   /* 39 -- not implemented syscall */
        zrt_gettimeofday,          /* 40 */
        zrt_clock,                 /* 41 */
        zrt_nanosleep,             /* 42 */
        zrt_nan,                   /* 43 -- not implemented syscall */
        zrt_nan,                   /* 44 -- not implemented syscall */
        zrt_nan,                   /* 45 -- not implemented syscall */
        zrt_nan,                   /* 46 -- not implemented syscall */
        zrt_nan,                   /* 47 -- not implemented syscall */
        zrt_nan,                   /* 48 -- not implemented syscall */
        zrt_nan,                   /* 49 -- not implemented syscall */
        zrt_nan,                   /* 50 -- not implemented syscall */
        zrt_nan,                   /* 51 -- not implemented syscall */
        zrt_nan,                   /* 52 -- not implemented syscall */
        zrt_nan,                   /* 53 -- not implemented syscall */
        zrt_nan,                   /* 54 -- not implemented syscall */
        zrt_nan,                   /* 55 -- not implemented syscall */
        zrt_nan,                   /* 56 -- not implemented syscall */
        zrt_nan,                   /* 57 -- not implemented syscall */
        zrt_nan,                   /* 58 -- not implemented syscall */
        zrt_nan,                   /* 59 -- not implemented syscall */
        zrt_imc_makeboundsock,     /* 60 */
        zrt_imc_accept,            /* 61 */
        zrt_imc_connect,           /* 62 */
        zrt_imc_sendmsg,           /* 63 */
        zrt_imc_recvmsg,           /* 64 */
        zrt_imc_mem_obj_create,    /* 65 */
        zrt_imc_socketpair,        /* 66 */
        zrt_nan,                   /* 67 -- not implemented syscall */
        zrt_nan,                   /* 68 -- not implemented syscall */
        zrt_nan,                   /* 69 -- not implemented syscall */
        zrt_mutex_create,          /* 70 */
        zrt_mutex_lock,            /* 71 */
        zrt_mutex_trylock,         /* 72 */
        zrt_mutex_unlock,          /* 73 */
        zrt_cond_create,           /* 74 */
        zrt_cond_wait,             /* 75 */
        zrt_cond_signal,           /* 76 */
        zrt_cond_broadcast,        /* 77 */
        zrt_nan,                   /* 78 -- not implemented syscall */
        zrt_cond_timed_wait_abs,   /* 79 */
        zrt_thread_create,         /* 80 */
        zrt_thread_exit,           /* 81 */
        zrt_tls_init,              /* 82 */
        zrt_thread_nice,           /* 83 */
        zrt_tls_get,               /* 84 */
        zrt_second_tls_set,        /* 85 */
        zrt_second_tls_get,        /* 86 */
        zrt_nan,                   /* 87 -- not implemented syscall */
        zrt_nan,                   /* 88 -- not implemented syscall */
        zrt_nan,                   /* 89 -- not implemented syscall */
        zrt_nan,                   /* 90 -- not implemented syscall */
        zrt_nan,                   /* 91 -- not implemented syscall */
        zrt_nan,                   /* 92 -- not implemented syscall */
        zrt_nan,                   /* 93 -- not implemented syscall */
        zrt_nan,                   /* 94 -- not implemented syscall */
        zrt_nan,                   /* 95 -- not implemented syscall */
        zrt_nan,                   /* 96 -- not implemented syscall */
        zrt_nan,                   /* 97 -- not implemented syscall */
        zrt_nan,                   /* 98 -- not implemented syscall */
        zrt_nan,                   /* 99 -- not implemented syscall */
        zrt_sem_create,            /* 100 */
        zrt_sem_wait,              /* 101 */
        zrt_sem_post,              /* 102 */
        zrt_sem_get_value,         /* 103 */
        zrt_dyncode_create,        /* 104 */
        zrt_dyncode_modify,        /* 105 */
        zrt_dyncode_delete,        /* 106 */
        zrt_nan,                   /* 107 -- not implemented syscall */
        zrt_nan,                   /* 108 -- not implemented syscall */
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
    setup_zrtlog_fd( zrt_log_fd );

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
    struct StreamReader* stream_reader = alloc_stream_reader( s_channels_mount, "/dev/tarimage" );

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
#endif
}

