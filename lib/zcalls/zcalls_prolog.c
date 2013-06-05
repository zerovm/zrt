
/* minimal implementation of zcalls interface used in section of code
 * running between prolog and begin of zrt initialization.*/


#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "zrtlog.h"
#include "zvm.h"
#include "zcalls.h"
#include "zcalls_zrt.h" //nvram()
#include "nvram_loader.h"
#include "fstab_observer.h"
#include "channels_reserved.h"
#include "environment_observer.h"
#include "args_observer.h"

#define SET_ERRNO(err) errno=err

#define ONLY_PROLOG_SYSCALL 1
//#define LOW_LEVEL_LOG_ENABLE

#define FUNC_NAME __func__

#ifdef LOW_LEVEL_LOG_ENABLE
#  define ZRT_LOG_LOW_LEVEL(str) \
    zrt_zcall_loglibc(str)
#else
#  define ZRT_LOG_LOW_LEVEL(str)
#endif //LOW_LEVEL_LOG_ENABLE


static int   s_prolog_doing_now;
static void* s_tls_addr=NULL;
static void* sbrk_default = NULL;


void zrt_zcall_prolog_init(){
    __zrt_log_init( ZRT_LOG_NAME );
    ZRT_LOG(L_INFO, P_TEXT, "prolog init");
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    s_prolog_doing_now = 1;
    if ( MANIFEST )
	sbrk_default = MANIFEST->heap_ptr;
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
	/*simplest implementation if trying to read file in case if
	  FS not accessible, using channels directly without checks
	  trying to read always from beginning*/
	if ( (*nread = zvm_pread(handle, buf, count, 0 )) >= 0 )
	    return 0; //read success
	else{
	    SET_ERRNO( *nread );
	    return -1; //read error
	}
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
	return zrt_zcall_enhanced_fstat(handle, stat);
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
	/*zrt not yet initialized while prolog init running
	 and any of FS not accessible only channles directly can be used.
	 just return file descriptor for file name if exist not checking
	flags or mode*/
	int fd;
	for (fd=0; fd < MANIFEST->channels_count; fd++ ){
	    if ( !strcmp( name, MANIFEST->channels[fd].name) ){
		*newfd = fd;
		errno=0;
		return 0; /*fd is matched*/
	    }
	}
	SET_ERRNO(ENOENT);
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

    if ( /* ONLY_PROLOG_SYSCALL ||  */s_prolog_doing_now ){
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
    /*very base implementation of tls handling*/
    s_tls_addr = thread_ptr;
    return 0;
}

void * zrt_zcall_prolog_tls_get(void){
    ZRT_LOG_LOW_LEVEL(FUNC_NAME);
    /*very base implementation of tls handling*/
    return s_tls_addr ; /*valid tls*/
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

int zrt_zcall_prolog_chdir(const char *path){
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
    s_prolog_doing_now = 0; 
    __zrt_log_prolog_mode_enable(0);
    ZRT_LOG_DELIMETER;
    zrt_zcall_enhanced_zrt_setup();    
}

/*@return records count in fstab section*/
static int get_records_count_for_section_and_buffer_size_to_copy_contents
(
 struct NvramLoader* nvram, const char* section_name, int* buf_size){
    /*Go through parsed envs section and calculate buffer size
      needed to store environment variables into single buffer
      as into null terminated strings folowing each after other*/
    ZRT_LOG(L_SHORT, "For nvram section '%s' calculations", section_name);
    int records_count = 0;
    struct ParsedRecords* section = nvram->section_by_name( nvram, section_name );
    if ( section != NULL ){
	struct ParsedRecord* current_rec;
	int j, k;
	for (j=0; j < section->count; j++){
	    /*calculate records count in section*/
	    ++records_count; 
	    current_rec = &section->records[j];
	    for (k=0; k < section->observer->keys.count; k++){
		struct ParsedParam* param = &section->records[j].parsed_params_array[k];
		*buf_size += param->vallen + 1; //+ null term char
	    }
	    ++(*buf_size); //fon null-termination char
	}
	/*reserve additional space to be sure, to be able add null termination 
	  chars for all available args, see args_observer.c: add_val_to_temp_buffer */
	(*buf_size)+= NVRAM_MAX_RECORDS_IN_SECTION; 
	ZRT_LOG(L_SHORT, "section records=%d, buf_size=%d", records_count, *buf_size);
	return records_count;
    }
    else
	return 0;
}

/*nvram access from prolog*/
void zrt_zcall_prolog_nvram_read_get_args_envs(int *args_buf_size, 
					       int *envs_buf_size, int *env_count){
    ZRT_LOG(L_SHORT, "nvram object size %u bytes", sizeof(struct NvramLoader));
    struct NvramLoader* nvram = static_nvram();
    construct_nvram_loader( nvram );
    /*Get static observers object, their memory should not be freed
     Must add here all observers to known nvram sections*/
    nvram->add_observer(nvram, get_fstab_observer() );
    nvram->add_observer(nvram, get_env_observer() );
    nvram->add_observer(nvram, get_arg_observer() );
    /*if readed not null bytes and result non negative then doing parsing*/
    if ( nvram->read(nvram, DEV_NVRAM) > 0 ){
	/*read and parse whole config strusture into NvramLoader object*/
        nvram->parse(nvram);
	/*Go through parsed envs section and calculate buffer size
	 needed to store environment variables into single buffer
	 as into null terminated strings folowing each after other*/
	*env_count = get_records_count_for_section_and_buffer_size_to_copy_contents
	    (nvram, ENVIRONMENT_SECTION_NAME, envs_buf_size);
	/*handle args section*/
	get_records_count_for_section_and_buffer_size_to_copy_contents
	    (nvram, ARGS_SECTION_NAME, args_buf_size);
    }
}

void zrt_zcall_prolog_nvram_get_args_envs(char** args, char* args_buf, int args_buf_size,
					  char** envs, char* envs_buf, int envs_buf_size){
    #define HANDLE_ONLY_ENV_SECTION get_env_observer()
    #define HANDLE_ONLY_ARG_SECTION get_arg_observer()
    struct NvramLoader* nvram = static_nvram();
    /*handle "env" section*/
    if ( NULL != nvram->section_by_name( nvram, ENVIRONMENT_SECTION_NAME ) ){
	ZRT_LOG(L_SHORT, "%s", "nvram handle envs");
	/*handle uses "envs_buf" to save envs data, 
	  and "envs" to get result as two-dimens array*/
	int handled_buf_index=0;
	nvram->handle(nvram, HANDLE_ONLY_ENV_SECTION, 
		      (void*)envs_buf, (void*)&envs_buf_size, (void*)&handled_buf_index );
	/*access to static data in environment_observer.c*/
	get_env_array(envs, envs_buf, handled_buf_index); 
    }
    /*handle "arg" section*/
    if ( NULL != nvram->section_by_name( nvram, ARGS_SECTION_NAME ) ){
	ZRT_LOG(L_SHORT, "%s", "nvram handle args");
	/*handle uses "args_buf" to save args data, 
	  and "args" to get result as two-dimens array*/
	int handled_buf_index=0;
	nvram->handle(nvram, HANDLE_ONLY_ARG_SECTION, 
		      (void*)args_buf, (void*)&args_buf_size, (void*)&handled_buf_index );
	/*access to static data in environment_observer.c*/
	get_arg_array(args, args_buf, handled_buf_index); 
    }
    else{
	/*if not provided section then explicitly set NULL*/
	args[0] = NULL;
	args[1] = NULL;
    }
}


