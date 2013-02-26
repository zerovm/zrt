
/* minimal implementation of zcalls interface used in section of code
 * running between prolog and begin of zrt initialization.*/

#include "zcalls.h"

#include <errno.h>
#include "zvm.h"
#define SET_ERRNO(err) errno=err

//#define LOW_LEVEL_LOG_ENABLE

#define LOW_LEVEL_LOG_FD 1
#define FUNC_NAME __func__

#ifdef LOW_LEVEL_LOG_ENABLE
#  define ZRT_LOG_LOW_LEVEL(str) \
    zvm_pwrite(LOW_LEVEL_LOG_FD, str, sizeof(str), 0); \
    zvm_pwrite(LOW_LEVEL_LOG_FD, "\n", sizeof("\n"), 0)
#else
#  define ZRT_LOG_LOW_LEVEL(str)
#endif //LOW_LEVEL_LOG_ENABLE


static int   s_prolog_doing_now;
static void* s_tls_addr=NULL;
static void* sbrk_default = NULL;

void zrt_zcall_prolog_init(){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    s_prolog_doing_now = 1;
    if ( zvm_init() )
	sbrk_default = zvm_init()->heap_ptr;
}

void zrt_zcall_prolog_exit(int status){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	zvm_exit(status); /*get controls into zerovm*/
	/* unreachable code*/
    }
    else
	zrt_zcall_enhanced_exit(status);
}

int  zrt_zcall_prolog_gettod(struct timeval *tvl){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_enhanced_gettod(tvl);
}
int  zrt_zcall_prolog_clock(clock_t *ticks){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*
     * should never be implemented if we want deterministic behaviour
     * note: but we can allow to return each time synthetic value
     * warning! after checking i found that nacl is not using it, so this
     *          function is useless for current nacl sdk version.
     */
    SET_ERRNO(EPERM);
    return -1;
}
int  zrt_zcall_prolog_nanosleep(const struct timespec *req, struct timespec *rem){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_sched_yield(void){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_sysconf(int name, int *value){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
/* irt fdio *************************/
int  zrt_zcall_prolog_close(int handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_enhanced_close(handle);
}
int  zrt_zcall_prolog_dup(int fd, int *newfd){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else 
	return zrt_zcall_enhanced_dup(fd, newfd);
}
int  zrt_zcall_prolog_dup2(int fd, int newfd){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_enhanced_dup2(fd, newfd);
}

int  zrt_zcall_prolog_read(int handle, void *buf, size_t count, size_t *nread){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else{
	return zrt_zcall_enhanced_read(handle, buf, count, nread);
    }
}

int  zrt_zcall_prolog_write(int handle, const void *buf, size_t count, size_t *nwrote){
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_enhanced_write(handle, buf, count, nwrote);
}

int  zrt_zcall_prolog_seek(int handle, off_t offset, int whence, off_t *new_offset){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_enhanced_seek(handle, offset, whence, new_offset);
}

int  zrt_zcall_prolog_fstat(int handle, struct stat *stat){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_prolog_fstat(handle, stat);
}

int  zrt_zcall_prolog_getdents(int fd, struct dirent *dirent_buf, size_t count, size_t *nread){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_enhanced_getdents(fd, dirent_buf, count, nread);
}

int  zrt_zcall_prolog_open(const char *name, int flags, mode_t mode, int *newfd){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_enhanced_open(name, flags, mode, newfd);
}

int  zrt_zcall_prolog_stat(const char *pathname, struct stat * stat){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_enhanced_stat(pathname, stat);
}

int  zrt_zcall_prolog_sysbrk(void **newbrk){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( *newbrk == NULL )
	*newbrk = sbrk_default;
    else
	sbrk_default = *newbrk;

    if ( s_prolog_doing_now ){
	return 0;
    }
    else
	return zrt_zcall_enhanced_sysbrk(newbrk);
}

int  zrt_zcall_prolog_mmap(void **addr, size_t length, int prot, int flags, int fd, off_t off){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_enhanced_mmap(addr, length, prot, flags, fd, off);
}

int  zrt_zcall_prolog_munmap(void *addr, size_t len){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	SET_ERRNO(ENOSYS);
	return -1;
    }
    else
	return zrt_zcall_enhanced_munmap(addr, len);
}

/* irt dyncode *************************/
int  zrt_zcall_prolog_dyncode_create(void *dest, const void *src, size_t size){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_dyncode_modify(void *dest, const void *src, size_t size){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_dyncode_delete(void *dest, size_t size){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
/* irt thread *************************/
int  zrt_zcall_prolog_thread_create(void *start_user_address, void *stack, void *thread_ptr){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
void zrt_zcall_prolog_thread_exit(int32_t *stack_flag){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return;
}
int  zrt_zcall_prolog_thread_nice(const int nice){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
/* irt mutex *************************/
int  zrt_zcall_prolog_mutex_create(int *mutex_handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_mutex_destroy(int mutex_handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_mutex_lock(int mutex_handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_mutex_unlock(int mutex_handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_mutex_trylock(int mutex_handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
/* irt cond *************************/
int  zrt_zcall_prolog_cond_create(int *cond_handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_cond_destroy(int cond_handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_cond_signal(int cond_handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_cond_broadcast(int cond_handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_cond_wait(int cond_handle, int mutex_handle){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_cond_timed_wait_abs(int cond_handle, int mutex_handle,
				   const struct timespec *abstime){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}

/* irt tls *************************/
int  zrt_zcall_prolog_tls_init(void *thread_ptr){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	s_tls_addr = thread_ptr;
	return 0;
    }
    else
	return zrt_zcall_enhanced_tls_init(thread_ptr);
}
void * zrt_zcall_prolog_tls_get(void){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    if ( s_prolog_doing_now ){
	return s_tls_addr ; /*valid tls*/
    }
    else
	return zrt_zcall_enhanced_tls_get();
}

/* irt resource open *************************/
int  zrt_zcall_prolog_open_resource(const char *file, int *fd){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
/* irt clock *************************/
int  zrt_zcall_prolog_getres(clockid_t clk_id, struct timespec *res){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}
int  zrt_zcall_prolog_gettime(clockid_t clk_id, struct timespec *tp){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*not implemented for both prolog and zrt enhanced */
    SET_ERRNO(ENOSYS);
    return -1;
}

/* Setup zrt */
void zrt_zcall_prolog_zrt_setup(void){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*prolog initialization done and now main syscall handling should be processed by
     *enhanced syscall handlers*/
    //s_prolog_doing_now = 0; 
    zrt_zcall_enhanced_zrt_setup();    
}

