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
#include <stdarg.h>
#include <unistd.h> //STDIN_FILENO
#include <fcntl.h> //file flags, O_ACCMODE
#include <errno.h>
#include <dirent.h>     /* Defines DT_* constants */
#include <assert.h>

#include "zvm.h"
#include "zrt.h"
#include "zrtsyscalls.h"
#include "memory.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "stream_reader.h"
#include "fstab_observer.h"
#include "fstab_loader.h"
#include "mounts_manager.h"
#include "mem_mount_wraper.h"
#include "nacl_struct.h"
#include "image_engine.h"
#include "channels_mount.h"
#include "enum_strings.h"


/* TODO
 * RESEARCH
 * #include <mcheck.h> research debugging capabilities
 * channels close, reopen, reclose
 * BUGS
 * fdopen failed, ftell fread
 * */

/*/dev/fstab support enable*/
#define FSTAB_CONF_ENABLE

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
static struct MemoryInterface* s_memory_interface;
/****************** */

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

static void debug_mes_stat(struct stat *stat){
    ZRT_LOG(L_INFO, 
	    "st_dev=%lld, st_ino=%lld, nlink=%d, st_mode=%o(octal), st_blksize=%d" 
	    "st_size=%lld, st_blocks=%d, st_atime=%lld, st_mtime=%lld", 
	    stat->st_dev, stat->st_ino, stat->st_nlink, stat->st_mode, (int)stat->st_blksize,
	    stat->st_size, (int)stat->st_blocks, stat->st_atime, stat->st_mtime );
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
    ZRT_LOG( L_SHORT, "mode=%o, umasked mode=%o", mode, umasked_mode );
    return umasked_mode;
}


/*************************************************************************
 * glibc substitution. Implemented functions below should be linked
 * instead of standard syscall that not implemented by NACL glibc
 **************************************************************************/

int mkdir(const char* pathname, mode_t mode){
    LOG_SYSCALL_START(NULL,0);
    errno=0;
    ZRT_LOG(L_SHORT, "pathname=%p, mode=%o(octal)", pathname, (uint32_t)mode);
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
    LOG_SYSCALL_START(NULL,0);
    errno=0;
    ZRT_LOG(L_SHORT, "pathname=%s", pathname);
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    char* absolute_path = alloc_absolute_path_from_relative( pathname );
    int ret = s_transparent_mount->rmdir( absolute_path );
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int lstat(const char *path, struct stat *buf){
    LOG_SYSCALL_START(NULL,0);
    errno=0;
    ZRT_LOG(L_SHORT, "path=%s, buf=%p", path, buf);
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
    LOG_SYSCALL_START(NULL,0);
    /*save new umask and return prev*/
    mode_t prev_umask = get_umask();
    char umask_str[11];
    sprintf( umask_str, "%o", mask );
    setenv( UMASK_ENV, umask_str, 1 );
    ZRT_LOG(L_SHORT, "%s", umask_str);
    LOG_SYSCALL_FINISH(0);
    return prev_umask;
}

int chown(const char *path, uid_t owner, gid_t group){
    LOG_SYSCALL_START(NULL,0);
    errno=0;
    ZRT_LOG(L_SHORT, "path=%s, owner=%u, group=%u", path, owner, group );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    char* absolute_path = alloc_absolute_path_from_relative(path);
    int ret = s_transparent_mount->chown(absolute_path, owner, group);
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int fchown(int fd, uid_t owner, gid_t group){
    LOG_SYSCALL_START(NULL,0);
    errno=0;
    ZRT_LOG(L_SHORT, "fd=%d, owner=%u, group=%u", fd, owner, group );
    int ret = s_transparent_mount->fchown(fd, owner, group);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int lchown(const char *path, uid_t owner, gid_t group){
    LOG_SYSCALL_START(NULL,0);
    ZRT_LOG(L_SHORT, "path=%s, owner=%u, group=%u", path, owner, group );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    /*do not do transformaton path, it's called in nested chown*/
    int ret =chown(path, owner, group);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int unlink(const char *pathname){
    LOG_SYSCALL_START(NULL,0);
    errno=0;
    ZRT_LOG(L_SHORT, "pathname=%s", pathname );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(pathname);
    char* absolute_path = alloc_absolute_path_from_relative(pathname);
    int ret = s_transparent_mount->unlink(absolute_path);
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/*todo: check if syscall chmod is supported by NACL then use it
*instead of this glibc substitution*/
int chmod(const char *path, mode_t mode){
    LOG_SYSCALL_START(NULL,0);
    errno=0;
    ZRT_LOG(L_SHORT, "path=%s, mode=%u", path, mode );
    VALIDATE_SUBSTITUTED_SYSCALL_PTR(path);
    mode = apply_umask(mode);
    char* absolute_path = alloc_absolute_path_from_relative(path);
    int ret = s_transparent_mount->chmod(absolute_path, mode);
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

int fchmod(int fd, mode_t mode){
    LOG_SYSCALL_START(NULL,0);
    errno=0;
    ZRT_LOG(L_SHORT, "fd=%d, mode=%u", fd, mode );
    mode = apply_umask(mode);
    int ret = s_transparent_mount->fchmod(fd, mode);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/*override system glibc implementation */
int fcntl(int fd, int cmd, ... /* arg */ ){
    LOG_SYSCALL_START(NULL,0);
    errno=0;
    ZRT_LOG(L_SHORT, "fd=%d, cmd=%u", fd, cmd );
    va_list args;
    va_start(args, cmd);
    int ret = s_transparent_mount->fcntl(fd, cmd, args);
    va_end(args);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/*substitude unsupported glibc implementation */
int remove(const char *pathname){
    LOG_SYSCALL_START(NULL,0);
    errno=0;
    ZRT_LOG(L_SHORT, "pathname=%s", pathname );
    int ret = s_transparent_mount->remove(pathname);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/*substitude unsupported glibc implementation */
int rename(const char *oldpath, const char *newpath){
    LOG_SYSCALL_START(NULL,0);
    int ret;
    errno=0;
    ZRT_LOG_PARAM(L_SHORT, P_TEXT, oldpath);
    ZRT_LOG_PARAM(L_SHORT, P_TEXT, newpath);
    struct stat oldstat;
    ret = stat(oldpath, &oldstat );
    if ( !ret ){
	ZRT_LOG(L_SHORT, "oldpath ok %d",1);
	struct stat newstat;
	char* absolute_path = alloc_absolute_path_from_relative(newpath);
	ret = s_transparent_mount->stat(absolute_path, &newstat);
	free(absolute_path);
	if ( ret == 0 || (ret != 0 && errno == ENOENT) ){
	    ZRT_LOG(L_SHORT, "newpath ok %d",1);
	    /*if oldpath exist and new filename does not exist, then
	     *read old file contents into buffer then create and write new file
	     *contents, close files, and remove old file from FS
	     */
	    char* oldbuf = malloc(oldstat.st_size);
	    int oldfd = open(oldpath, O_RDONLY);
	    int bytes = read(oldfd, oldbuf, oldstat.st_size);
	    close(oldfd);
	    ZRT_LOG(L_EXTRA, "bytes=%d, st_size=%d", bytes, (int)oldstat.st_size);
	    assert(bytes==oldstat.st_size);
	    /*if new file path exist then remove it*/
	    if ( ret == 0 ){
		remove(newpath);
	    }
	    /*create new file*/
	    int newfd = open(newpath, O_CREAT | O_WRONLY);
	    if ( newfd >=0 ){
		int bytes_w = write(newfd, oldbuf, oldstat.st_size);
		close(newfd);
		ZRT_LOG(L_EXTRA, "bytes_w=%d, st_size=%d", bytes, (int)oldstat.st_size);
		assert(bytes_w==oldstat.st_size);
		remove(oldpath);
		ret=0; /*rename success*/
	    }
	    free(oldbuf);
	}
    }

    LOG_SYSCALL_FINISH(ret);
    return ret;
}

/*substitude unsupported glibc implementation */
/* FILE *fdopen(int fd, const char *mode){ */
/*     LOG_SYSCALL_START(NULL,0); */
/*     ZRT_LOG_PARAM(L_SHORT, P_INT, fd); */
/*     ZRT_LOG_PARAM(L_SHORT, P_TEXT, mode); */
/*     LOG_SYSCALL_FINISH(0); */
/*     return NULL; */
/* } */

/*override system glibc implementation due to bad errno at errors*/
/* int fseek(FILE *stream, long offset, int whence){ */
/*     LOG_SYSCALL_START(NULL); */
/*     errno = 0; */
/*     int handle = fileno(stream); */
/*     int ret = s_transparent_mount->lseek(handle, offset, whence); */
/*     LOG_SYSCALL_FINISH(ret); */
/*     return ret; */
/* } */


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
    LOG_SYSCALL_START(args,3);
    errno=0;
    char* name = (char*)args[0];
    int flags = (int)args[1];
    uint32_t mode = (int)args[2];
    ZRT_LOG_PARAM(L_SHORT, P_TEXT, name);
    VALIDATE_SYSCALL_PTR(name);
    ZRT_LOG_PARAM(L_SHORT, P_TEXT, FILE_OPEN_FLAGS(flags));
    
    char* absolute_path = alloc_absolute_path_from_relative( name );
    mode = apply_umask(mode);
    ZRT_LOG_PARAM(L_SHORT, P_TEXT, FILE_OPEN_MODE(mode));
    int ret = s_transparent_mount->open( absolute_path, flags, mode );
    free(absolute_path);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}


/* do nothing but checks given handle */
static int32_t zrt_close(uint32_t *args)
{
    LOG_SYSCALL_START(args,1);
    errno = 0;
    int handle = (int)args[0];
    ZRT_LOG_PARAM(L_SHORT, P_INT, handle);

    int ret = s_transparent_mount->close(handle);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}


/* read the file with the given handle number */
static int32_t zrt_read(uint32_t *args)
{
    LOG_SYSCALL_START(args,3);
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
    LOG_SYSCALL_START(args,3);
    int handle = (int)args[0];
    void *buf = (void*)args[1];
    VALIDATE_SYSCALL_PTR(buf);
    int64_t length = (int64_t)args[2];

#ifdef DEBUG
    /*disable logging write calls related to debug, stdout and stderr channel */
    if ( handle == zrtlog_fd() ){
        disable_logging_current_syscall();
    }
#endif

    int32_t ret = s_transparent_mount->write(handle, buf, length);
    LOG_SYSCALL_FINISH(ret);
    return ret;
}

static int32_t zrt_lseek(uint32_t *args)
{
    LOG_SYSCALL_START(args,2);
    errno = 0;
    int32_t handle = (int32_t)args[0];
    off_t offset = *((off_t*)args[1]);
    int whence = (int)args[2];

    ZRT_LOG_PARAM(L_SHORT, P_LONGINT, offset);
    ZRT_LOG_PARAM(L_SHORT, P_TEXT, SEEK_WHENCE(whence));


    if ( whence == SEEK_SET && offset < 0 ){
	SET_ERRNO(EINVAL);
	offset=-1;
    }
    else{
	offset = s_transparent_mount->lseek(handle, offset, whence);
    }

    *(off_t *)args[1] = offset;
    LOG_SYSCALL_FINISH(offset);
    return offset;
}


SYSCALL_MOCK(ioctl, -EINVAL) /* not implemented in the simple version of zrtlib */


static int32_t zrt_stat(uint32_t *args)
{
    LOG_SYSCALL_START(args,2);
    errno = 0;
    const char *file = (const char*)args[0];
    struct nacl_abi_stat *sbuf = (struct nacl_abi_stat *)args[1];
    VALIDATE_SYSCALL_PTR(file);
    VALIDATE_SYSCALL_PTR(sbuf);
    ZRT_LOG(L_SHORT, "file=%s", file);
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


static int32_t zrt_fstat(uint32_t *args)
{
    LOG_SYSCALL_START(args,2);
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
    LOG_SYSCALL_START(args,1);
    int32_t retaddr = s_memory_interface->sysbrk(s_memory_interface, (void*)args[0] );
    LOG_SYSCALL_FINISH(retaddr);
    return retaddr;
}

/* map region of memory. ZRT simple implementation;*/
static int32_t zrt_mmap(uint32_t *args)
{
    LOG_SYSCALL_START(args,6);
    int32_t retcode = -1;
    void* addr = (void*)args[0];
    uint32_t length = args[1];
    uint32_t prot = args[2];
    uint32_t flags = args[3];
    uint32_t fd = args[4];
    off_t offset = (off_t)args[5];

    ZRT_LOG(L_INFO, "mmap prot=%s, mmap flags=%s", 
	    MMAP_PROT_FLAGS(prot), MMAP_FLAGS(flags));
    retcode = s_memory_interface->mmap(s_memory_interface, addr, length, prot, 
		  flags, fd, offset);
  
    LOG_SYSCALL_FINISH(retcode);
    return retcode;
}

static int32_t zrt_munmap(uint32_t *args)
{
    LOG_SYSCALL_START(args,2);
    int32_t retcode = s_memory_interface->munmap(s_memory_interface, 
						 (void*)args[0], args[1]);
    LOG_SYSCALL_FINISH(retcode);
    return retcode;
}


static int32_t zrt_getdents(uint32_t *args)
{
    LOG_SYSCALL_START(args,3);
    errno=0;
    int handle = (int)args[0];
    char *buf = (char*)args[1];
    VALIDATE_SYSCALL_PTR(buf);
    uint32_t count = args[2];

    int32_t bytes_readed = s_transparent_mount->getdents(handle, buf, count);
    LOG_SYSCALL_FINISH(bytes_readed);
    return bytes_readed;
}


/*
 * exit. most important syscall. without it the user program
 * cannot terminate correctly.
 */
static int32_t zrt_exit(uint32_t *args)
{
    /* no need to check args for NULL. it is always set by syscall_manager */
    LOG_SYSCALL_START(args,1);
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
    //LOG_SYSCALL_START(args,1);
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
        ZRT_LOG(L_INFO,
		"tv->nacl_abi_tv_sec=%lld, tv->nacl_abi_tv_usec=%d", 
		tv->nacl_abi_tv_sec, tv->nacl_abi_tv_usec );

        /* update time value*/
        update_cached_time();
    }

    //LOG_SYSCALL_FINISH(ret);
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
     * LOG_SYSCALL_START(args,1);*/
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
    LOG_SYSCALL_START(args,6);
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
    int ZRT_LOG_fd = s_channels_mount->open( ZRT_LOG_NAME, O_WRONLY, 0 ); /*open log channel*/
    set_zrtlog_fd( ZRT_LOG_fd );

    s_memory_interface = memory_interface( manifest->heap_ptr, manifest->heap_size );
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
        ZRT_LOG(L_SHORT, 
		"s_cached_timeval.nacl_abi_tv_sec=%lld", 
		s_cached_timeval.tv_sec );
    }

    /*create mem mount*/
    s_mem_mount = alloc_mem_mount( s_mounts_manager->handle_allocator );

    /*Mount filesystems*/
    s_mounts_manager->mount_add( "/dev", s_channels_mount );
    s_mounts_manager->mount_add( "/", s_mem_mount );

    /*explicitly create /dev directory in memmount, it's required for consistent
      FS structure, readdir from now can list /dev dir recursively from root */
    s_mem_mount->mkdir( "/dev", 0777 );

#ifdef FSTAB_CONF_ENABLE
    /*Get static fstab observer object, it's memory should not be freed*/
    struct MFstabObserver* fstab_observer = get_fstab_observer();
    /*read fstab configuration*/
    struct FstabLoader* fstab = alloc_fstab_loader( s_channels_mount, s_transparent_mount );
    /*if readed not null bytes and result not negative then doing parsing*/
    if ( fstab->read(fstab, DEV_FSTAB) > 0 ){
        int res = fstab->parse(fstab, fstab_observer);
	ZRT_LOG(L_SHORT, "fstab parse res=%d", res);
    }

    free_fstab_loader(fstab);
    ZRT_LOG_DELIMETER;
#endif
}


