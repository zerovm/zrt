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
#include "zrt_config.h"
#include "zcalls_zrt.h"
#include "memory_syscall_handlers.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "stream_reader.h"
#include "path_utils.h"             /*alloc_absolute_path_from_relative*/
#include "fstab_observer.h"
#include "nvram_loader.h"
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

#ifdef DEBUG
#define ZRT_LOG_NAME "/dev/debug"
#endif //DEBUG

/* if given in manifest let user to have it */
#define TIMESTAMP_STR "TimeStamp"


/****************** static data*/
static int                     s_tls_cache=-1;
struct timeval                 s_cached_timeval;
struct MountsInterface*        s_channels_mount=NULL;
struct MountsInterface*        s_mem_mount=NULL;
static struct MountsManager* s_mounts_manager = NULL;
static struct MountsInterface* s_transparent_mount = NULL;
static struct MemoryInterface* s_memory_interface = NULL;
/****************** */

struct MountsInterface* transparent_mount() { return s_transparent_mount; }


/***********************************************************
 *ZRT initializators
 ***********************************************************/

/*first step zrt initializer*/
static void zrt_setup( struct UserManifest* manifest ){
    /*manage mounted filesystems*/
    s_mounts_manager = get_mounts_manager();

    /*alloc filesystem based on channels*/
    s_channels_mount = alloc_channels_mount( s_mounts_manager->handle_allocator,
					     manifest->channels, manifest->channels_count );
    /*alloc entry point to mounted filesystems*/
    s_transparent_mount = alloc_transparent_mount( s_mounts_manager );

    /* using direct call to channels_mount and create debuging log channel*/
    int zrtlog_fd = s_channels_mount->open( ZRT_LOG_NAME, O_WRONLY, 0 ); /*open log channel*/
    __zrt_log_set_fd( zrtlog_fd );
    __zrt_log_enable(1);
}

/*second step zrt initializer*/
static void zrt_setup_finally(){
    /* using of channels_mount directly to preallocate standard channels sdtin, stdout, stderr */
    /* logPushFilter(name,canberead,canbewrite,handle); */
    s_channels_mount->open( DEV_STDIN, O_RDONLY, 0 );
    s_channels_mount->open( DEV_STDOUT, O_WRONLY, 0 );
    s_channels_mount->open( DEV_STDERR, O_WRONLY, 0 );
    /* logPopFilter(); */

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
    struct MNvramObserver* fstab_observer = get_fstab_observer();
    struct NvramLoader* nvram = alloc_nvram_loader( s_channels_mount, s_transparent_mount );
    /*add observers here to handle various sections of config data*/
    nvram->add_observer(nvram, fstab_observer);
    /*if readed not null bytes and result not negative then doing parsing*/
    if ( nvram->read(nvram, DEV_FSTAB) > 0 ){
        int res = nvram->parse(nvram);
	ZRT_LOG(L_SHORT, "nvram parse res=%d", res);
    }

    fstab_observer->cleanup( fstab_observer );
    nvram->free( nvram );
    ZRT_LOG_DELIMETER;
#endif
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

void debug_mes_stat(struct stat *stat){
    ZRT_LOG(L_INFO, 
	    "st_dev=%lld, st_ino=%lld, nlink=%d, st_mode=%o(octal), st_blksize=%d" 
	    "st_size=%lld, st_blocks=%d, st_atime=%lld, st_mtime=%lld", 
	    stat->st_dev, stat->st_ino, stat->st_nlink, stat->st_mode, (int)stat->st_blksize,
	    stat->st_size, (int)stat->st_blocks, stat->st_atime, stat->st_mtime );
}

/*move it from here*/
mode_t get_umask(){
    mode_t prev_umask=0;
    const char* prev_umask_str = getenv(UMASK_ENV);
    if ( prev_umask_str ){
        sscanf( prev_umask_str, "%o", &prev_umask);
    }
    return prev_umask;
}

/*move it from here*/
mode_t apply_umask(mode_t mode){
    mode_t umasked_mode = ~get_umask() & mode; /*reset umask bits for specified mode*/
    ZRT_LOG( L_INFO, "mode=%o, umasked mode=%o", mode, umasked_mode );
    return umasked_mode;
}


/********************************************************************************
 * ZRT IMPLEMENTATION OF NACL SYSCALLS
 * each nacl syscall must be implemented or, at least, mocked. no exclusions!
 *********************************************************************************/

/* irt basic *************************/

/*
 * exit. without it the user program cannot terminate correctly.
 */
void zrt_zcall_enhanced_exit(int status){
    LOG_SYSCALL_START(P_HEX, status);
    ZRT_LOG(L_SHORT, P_TEXT, "exiting...");
    zvm_exit(status); /*get controls into zerovm*/
    /* unreachable code*/
    return; 
}

int  zrt_zcall_enhanced_gettod(struct timeval *tvl){
    struct nacl_abi_timeval  *tv = (struct nacl_abi_timeval *)tvl;
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

    return ret;
}
int  zrt_zcall_enhanced_clock(clock_t *ticks){
    SET_ERRNO(EPERM);
    return -1;
}
int  zrt_zcall_enhanced_nanosleep(const struct timespec *req, struct timespec *rem){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_sched_yield(void){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_sysconf(int name, int *value){
    SET_ERRNO(ENOSYS);
    return -1;
}
/* irt fdio *************************/
int  zrt_zcall_enhanced_close(int handle){
    LOG_SYSCALL_START("handle=%d", handle);
    errno = 0;

    int ret = s_transparent_mount->close(handle);
    LOG_SHORT_SYSCALL_FINISH( ret, "handle=%d", handle);
    return ret;
}
int  zrt_zcall_enhanced_dup(int fd, int *newfd){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_dup2(int fd, int newfd){
    SET_ERRNO(ENOSYS);
    return -1;
}

int  zrt_zcall_enhanced_read(int handle, void *buf, size_t count, size_t *nread){
    LOG_SYSCALL_START("handle=%d buf=%p count=%u", handle, buf, count);
    errno = 0;
    VALIDATE_SYSCALL_PTR(buf);

    int32_t ret = s_transparent_mount->read(handle, buf, count);
    /*get read bytes by pointer*/
    *nread = ret;
    LOG_SHORT_SYSCALL_FINISH( ret, "handle=%d", handle);
    return ret;
}

int  zrt_zcall_enhanced_write(int handle, const void *buf, size_t count, size_t *nwrote){
    /*disable logging write calls related to debug, stdout and stderr channel */
    if ( handle != __zrt_log_fd() ){
	LOG_SYSCALL_START("handle=%d buf=%p count=%u", handle, buf, count);
    }
    VALIDATE_SYSCALL_PTR(buf);

    int32_t ret = s_transparent_mount->write(handle, buf, count);
    /*get wrote bytes by pointer*/
    *nwrote = ret;
    if ( handle != __zrt_log_fd() ){
	LOG_SHORT_SYSCALL_FINISH( ret, "handle=%d count=%u", handle, count);
    }
    return ret;
}

int  zrt_zcall_enhanced_seek(int handle, off_t offset, int whence, off_t *new_offset){
    LOG_SYSCALL_START("handle=%d offset=%lld whence=%d", handle, offset, whence);
    errno = 0;

    if ( whence == SEEK_SET && offset < 0 ){
	SET_ERRNO(EINVAL);
	offset=-1;
    }
    else{
	offset = s_transparent_mount->lseek(handle, offset, whence);
    }

    /*get new offset by pointer*/
    *new_offset = offset;
    LOG_SHORT_SYSCALL_FINISH( offset, 
		       "handle=%d whence=%s", 
		       handle, STR_SEEK_WHENCE(whence));
    return offset;
}

int  zrt_zcall_enhanced_fstat(int handle, struct stat *stat){
    struct nacl_abi_stat *sbuf = (struct nacl_abi_stat *)stat;
    LOG_SYSCALL_START("handle=%d sbuf=%p", handle, sbuf);
    errno = 0;
    VALIDATE_SYSCALL_PTR(sbuf);

    struct stat st;
    int ret = s_transparent_mount->fstat( handle, &st);
    if ( ret == 0 ){
        debug_mes_stat(&st);
        set_nacl_stat( &st, sbuf ); //convert from nacl_stat into stat
    }
    LOG_SHORT_SYSCALL_FINISH( ret, "handle=%d", handle);
    return ret;
}

int  zrt_zcall_enhanced_getdents(int fd, struct dirent *dirent_buf, size_t count, size_t *nread){
    LOG_SYSCALL_START("fd=%d dirent_buf=%p count=%u", fd, dirent_buf, count);
    errno=0;
    VALIDATE_SYSCALL_PTR(dirent_buf);

    int32_t bytes_readed = s_transparent_mount->getdents(fd, (char*)dirent_buf, count);
    LOG_SHORT_SYSCALL_FINISH( bytes_readed, "fd=%d count=%u", fd, count);
    return bytes_readed;
}

int  zrt_zcall_enhanced_open(const char *name, int flags, mode_t mode, int *newfd){
    LOG_SYSCALL_START("name=%s flags=%d mode=%u", name, flags, mode );
    errno=0;
    VALIDATE_SYSCALL_PTR(name);
    
    char* absolute_path = alloc_absolute_path_from_relative( name );
    mode = apply_umask(mode);
    int ret = s_transparent_mount->open( absolute_path, flags, mode );
    free(absolute_path);
    /*get fd by pointer*/
    if ( ret >= 0 ) *newfd  = ret;
    LOG_SHORT_SYSCALL_FINISH( ret, 
		       "name=%s, flags=%s", 
		       name, STR_FILE_OPEN_FLAGS(flags));
    return ret;
}

int  zrt_zcall_enhanced_stat(const char *pathname, struct stat * stat){
    LOG_SYSCALL_START("pathname=%s stat=%p", pathname, stat);
    errno = 0;
    VALIDATE_SYSCALL_PTR(pathname);
    VALIDATE_SYSCALL_PTR(stat);

    struct nacl_abi_stat *sbuf = (struct nacl_abi_stat *)stat;
    struct stat st;
    char* absolute_path = alloc_absolute_path_from_relative(pathname);
    int ret = s_transparent_mount->stat(absolute_path, &st);
    free(absolute_path);
    if ( ret == 0 ){
        debug_mes_stat(&st);
        set_nacl_stat( &st, sbuf ); //convert from nacl_stat into stat
    }
    LOG_SHORT_SYSCALL_FINISH( ret, "pathname=%s", pathname);
    return ret;
}

/*
 * following 3 functions (sysbrk, mmap, munmap) is the part of the
 * new memory engine. the new allocate specified amount of ram before
 * user code start and then user can only obtain that allocated memory.
 * zrt lib will help user to do it transparently.
 */

/* irt memory *************************/
int  zrt_zcall_enhanced_sysbrk(void **newbrk){
    LOG_SYSCALL_START("*newbrk=%p", *newbrk);
    int32_t retaddr = s_memory_interface->sysbrk(s_memory_interface, *newbrk );
    /*get new address via pointer*/
    *newbrk = (void*)retaddr;
    LOG_INFO_SYSCALL_FINISH( retaddr, "param=%p", *newbrk);
    return retaddr;
}

int  zrt_zcall_enhanced_mmap(void **addr, size_t length, int prot, int flags, int fd, off_t off){
    int32_t retcode = -1;
    LOG_SYSCALL_START("addr=%p length=%u prot=%u flags=%u fd=%u off=%lld",
    		      *addr, length, prot, flags, fd, off);

    retcode = s_memory_interface->mmap(s_memory_interface, *addr, length, prot,
    		  flags, fd, off);
  
    LOG_INFO_SYSCALL_FINISH( retcode,
    		       "addr=%p length=%u prot=%s flags=%s fd=%u off=%lld",
    		       *addr, length, STR_MMAP_PROT_FLAGS(prot), STR_MMAP_FLAGS(flags),
    		       fd, off);
    return retcode;
}

int  zrt_zcall_enhanced_munmap(void *addr, size_t len){
    LOG_SYSCALL_START("addr=%p, len=%u", addr, len);
    int32_t retcode = s_memory_interface->munmap(s_memory_interface, addr, len);
    LOG_INFO_SYSCALL_FINISH( retcode, "addr=%p, len=%u", addr, len);
    return retcode;
}

/* irt dyncode *************************/
int  zrt_zcall_enhanced_dyncode_create(void *dest, const void *src, size_t size){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_dyncode_modify(void *dest, const void *src, size_t size){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_dyncode_delete(void *dest, size_t size){
    SET_ERRNO(ENOSYS);
    return -1;
}
/* irt thread *************************/
int  zrt_zcall_enhanced_thread_create(void *start_user_address, void *stack, void *thread_ptr){
    SET_ERRNO(ENOSYS);
    return -1;
}
void zrt_zcall_enhanced_thread_exit(int32_t *stack_flag){
    SET_ERRNO(ENOSYS);
}
int  zrt_zcall_enhanced_thread_nice(const int nice){
    SET_ERRNO(ENOSYS);
    return -1;
}
/* irt mutex *************************/
int  zrt_zcall_enhanced_mutex_create(int *mutex_handle){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_mutex_destroy(int mutex_handle){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_mutex_lock(int mutex_handle){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_mutex_unlock(int mutex_handle){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_mutex_trylock(int mutex_handle){
    SET_ERRNO(ENOSYS);
    return -1;
}
/* irt cond *************************/
int  zrt_zcall_enhanced_cond_create(int *cond_handle){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_cond_destroy(int cond_handle){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_cond_signal(int cond_handle){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_cond_broadcast(int cond_handle){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_cond_wait(int cond_handle, int mutex_handle){
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_enhanced_cond_timed_wait_abs(int cond_handle, int mutex_handle,
				   const struct timespec *abstime){
    SET_ERRNO(ENOSYS);
    return -1;
}

/* irt tls *************************/
int  zrt_zcall_enhanced_tls_init(void *thread_ptr){
    LOG_SYSCALL_START("thread_ptr=%p", thread_ptr);
    s_tls_cache = (uintptr_t)thread_ptr;
    LOG_INFO_SYSCALL_FINISH( 0, "thread_ptr=%p", thread_ptr);
    return 0;
}
void * zrt_zcall_enhanced_tls_get(void){
    /* tls_get return value would be cached because it remains unchanged 
     * after setup by tls_init call*/
    if ( s_tls_cache < 0 ){
	assert(s_tls_cache>0);
    }
    return (void*)s_tls_cache; /*valid tls*/
}

/************************************************************************
* zcalls_zrt_t implementation
* Setup zrt
*************************************************************************/
void zrt_zcall_enhanced_zrt_setup(void){
    int i;
    struct UserManifest *setup = zvm_init();
    s_memory_interface = get_memory_interface( setup->heap_ptr, setup->heap_size );

    /* /\* /\\*test code*\\/ *\/ */
    /* malloc(2); */
    /* exit(-1); */
    /* s_mounts_manager = get_mounts_manager(); */
    /* s_channels_mount = alloc_channels_mount( s_mounts_manager->handle_allocator, */
    /* 					     setup->channels, setup->channels_count ); */

    /* exit(0); */
    /* /\* /\\*test code*\\/ *\/ */
    zrt_setup( setup );

    /* debug print */
    ZRT_LOG(L_SHORT, "DEBUG INFORMATION%s", "");
    ZRT_LOG(L_SHORT, "user heap pointer address = 0x%x", (intptr_t)setup->heap_ptr);
    ZRT_LOG(L_SHORT, "user memory size = %u", setup->heap_size);
    ZRT_LOG_DELIMETER;
    ZRT_LOG(L_SHORT, "sizeof(struct ZVMChannel) = %d", sizeof(struct ZVMChannel));
    ZRT_LOG(L_SHORT, "channels count = %d", setup->channels_count);
    ZRT_LOG_DELIMETER;
    /*print channels list*/
    for(i = 0; i < setup->channels_count; ++i)
    {
        ZRT_LOG(L_SHORT, "channel[%d].name = '%s'", i, setup->channels[i].name);
        ZRT_LOG(L_SHORT, "channel[%d].type = %d", i, setup->channels[i].type);
        ZRT_LOG(L_SHORT, "channel[%d].size = %lld", i, setup->channels[i].size);
        ZRT_LOG(L_SHORT, "channel[%d].limits[GetsLimit] = %lld", i, setup->channels[i].limits[GetsLimit]);
        ZRT_LOG(L_SHORT, "channel[%d].limits[GetSizeLimit] = %lld", i, setup->channels[i].limits[GetSizeLimit]);
        ZRT_LOG(L_SHORT, "channel[%d].limits[PutsLimit] = %lld", i, setup->channels[i].limits[PutsLimit]);
        ZRT_LOG(L_SHORT, "channel[%d].limits[PutSizeLimit] = %lld", i, setup->channels[i].limits[PutSizeLimit]);
    }
    ZRT_LOG_DELIMETER;
    ZRT_LOG(L_SHORT, "_SC_PAGE_SIZE=%ld", sysconf(_SC_PAGE_SIZE));

    zrt_setup_finally();
}

/*************************************************************************/



