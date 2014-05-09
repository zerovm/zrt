/*
 * zcalls_zrt.c
 * Complete implementation of syscall handlers, available after 
 * prolog stage fully passed, and heap memory is available;
 * In case if enhanced syscall handler does not exist then use
 * Basic handler, implemented at zcall_prolog.c;
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
#include "zrtapi.h"
#include "zrt_defines.h"
#include "zcalls.h"
#include "zcalls_zrt.h"
#include "memory_syscall_handlers.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "parse_path.h"
#include "mounts_reader.h"
#include "settime_observer.h"
#include "args_observer.h"
#include "environment_observer.h"
#include "fstab_observer.h"
#include "mapping_observer.h"
#include "precache_observer.h"
#include "debug_observer.h"
#include "nvram_loader.h"
#include "mounts_manager.h"
#include "mem_mount_wraper.h"
#include "nacl_struct.h"
#include "image_engine.h"
#include "enum_strings.h"
#include "channels_reserved.h"
#include "channels_mount.h"

extern char **environ;

#define CHANNEL_SIZE_LIMIT 999999999
#define CHANNEL_OPS_LIMIT  999999999
/****************** static data*/
static struct ZVMChannel s_emu_channels[] 
= { {{CHANNEL_OPS_LIMIT, CHANNEL_SIZE_LIMIT,CHANNEL_OPS_LIMIT, CHANNEL_SIZE_LIMIT},0,SGetSPut,"/dev/null"},
    {{CHANNEL_OPS_LIMIT, CHANNEL_SIZE_LIMIT,CHANNEL_OPS_LIMIT, CHANNEL_SIZE_LIMIT},0,SGetSPut,"/dev/full"},
    {{CHANNEL_OPS_LIMIT, CHANNEL_SIZE_LIMIT,CHANNEL_OPS_LIMIT, CHANNEL_SIZE_LIMIT},0,SGetSPut,"/dev/zero"},
    {{CHANNEL_OPS_LIMIT, CHANNEL_SIZE_LIMIT,CHANNEL_OPS_LIMIT, CHANNEL_SIZE_LIMIT},0,SGetSPut,"/dev/random"},
    {{CHANNEL_OPS_LIMIT, CHANNEL_SIZE_LIMIT,CHANNEL_OPS_LIMIT, CHANNEL_SIZE_LIMIT},0,SGetSPut,"/dev/urandom"}};

struct MountsPublicInterface*        s_channels_mount;
struct MountsPublicInterface*        s_mem_mount;
static struct MountsManager*   s_mounts_manager;
static struct MountsPublicInterface* s_transparent_mount;
static int                     s_zrt_ready;
/****************** */

struct MountsPublicInterface* transparent_mount() { return s_transparent_mount; }

/*internal functions to be used in this module*/
void zrt_internal_session_info();
void zrt_internal_init( const struct UserManifest const* manifest );

void set_home_dir(const char *home)
{
    if ( home != NULL ) {
	int err;
	if ( !(err=mkpath_recursively(home, 0666)) || (err==-1&&errno==EEXIST) ){
	    if( chdir(home) != 0 ){
		ZRT_LOG(L_ERROR, "Error setting home directory %s, errno=%d", home, errno);
	    }
	}
	else{
	    ZRT_LOG(L_ERROR, "Error creating home directory %s, errno=%d", home, errno);
	}
    }
}

/********************************************************************************
 * ZRT IMPLEMENTATION OF ZCALLS
 * each zsyscall must be implemented or, at least, mocked. no exclusions!
 *********************************************************************************/

/* irt basic *************************/

/*
 * exit. without it the user program cannot terminate correctly.
 */
void zrt_zcall_enhanced_exit(int status){
    ZRT_LOG(L_SHORT, "status %d exiting...", status);
    get_fstab_observer()->mount_export(HANDLE_ONLY_FSTAB_SECTION);
    zvm_exit(status); /*get controls into zerovm*/
    /* unreachable code*/
    return; 
}

/* irt fdio *************************/
int  zrt_zcall_enhanced_close(int handle){
    LOG_SYSCALL_START("handle=%d", handle);
    errno = 0;

    int ret = s_transparent_mount->close(s_transparent_mount,handle);
    LOG_SHORT_SYSCALL_FINISH( ret, "handle=%d", handle);
    return ret;
}

int  zrt_zcall_enhanced_dup(int handle){
    LOG_SYSCALL_START("handle=%d", handle);
    errno = 0;

    int ret = s_transparent_mount->dup(s_transparent_mount, handle);
    LOG_SHORT_SYSCALL_FINISH( ret, "handle=%d", handle);
    return ret;
}

int  zrt_zcall_enhanced_dup2(int handle, int new_handle){
    LOG_SYSCALL_START("handle=%d", handle);
    errno = 0;

    int ret = s_transparent_mount->dup2(s_transparent_mount, handle, new_handle);
    LOG_SHORT_SYSCALL_FINISH( ret, "handle=%d", handle);
    return ret;
}

int  zrt_zcall_enhanced_read(int handle, void *buf, size_t count, size_t *nread){
    int ret=-1;
    LOG_SYSCALL_START("handle=%d buf=%p count=%u", handle, buf, count);
    errno = 0;
    VALIDATE_SYSCALL_PTR(buf);

    int32_t bytes_read = s_transparent_mount->read(s_transparent_mount,handle, buf, count);
    if ( bytes_read >= 0 ){
	/*get read bytes by pointer*/
	*nread = bytes_read;
	ret = 0;
    }
    LOG_INFO_SYSCALL_FINISH( ret, "bytes_read=%d, handle=%d", bytes_read, handle);
    return ret;
}

int  zrt_zcall_enhanced_write(int handle, const void *buf, size_t count, size_t *nwrote){
    int ret=-1;
    int log_state = __zrt_log_is_enabled();
    /*disable logging while writing to stdout and stderr channel */
    if ( handle <= 2 && log_state ){
	__zrt_log_enable(0);
    }

    LOG_SYSCALL_START("handle=%d buf=%p count=%u", handle, buf, count);
    VALIDATE_SYSCALL_PTR(buf);

    int32_t bytes_wrote = s_transparent_mount->write(s_transparent_mount,handle, buf, count);
    if ( bytes_wrote >= 0 ){
	/*get wrote bytes by pointer*/
	*nwrote = bytes_wrote;
	ret=0;
    }
    LOG_INFO_SYSCALL_FINISH( ret, "bytes_wrote=%d, handle=%d count=%u", 
			     bytes_wrote, handle, count);
    if ( log_state )
	__zrt_log_enable(log_state); /*restore log state*/
    return ret;
}


////////////////
int  zrt_zcall_enhanced_pread(int handle, void *buf, size_t count, off_t offset,
			      size_t *nread){
    int ret=-1;
    LOG_SYSCALL_START("handle=%d buf=%p count=%u, offset=%lld", 
		      handle, buf, count, offset);
    errno = 0;
    VALIDATE_SYSCALL_PTR(buf);

    int32_t bytes_read = s_transparent_mount->pread(s_transparent_mount,
						    handle, buf, count, offset);
    if ( bytes_read >= 0 ){
	/*get read bytes by pointer*/
	*nread = bytes_read;
	ret = 0;
    }
    LOG_INFO_SYSCALL_FINISH( ret, "bytes_read=%d, handle=%d", bytes_read, handle);
    return ret;
}

int  zrt_zcall_enhanced_pwrite(int handle, const void *buf, size_t count, off_t offset,
			       size_t *nwrote){
    int ret=-1;
    int log_state = __zrt_log_is_enabled();
    /*disable logging while writing to stdout and stderr channel */
    if ( handle <= 2 && log_state ){
	__zrt_log_enable(0);
    }

    LOG_SYSCALL_START("handle=%d buf=%p count=%u, offset=%lld", 
		      handle, buf, count, offset);
    VALIDATE_SYSCALL_PTR(buf);

    int32_t bytes_wrote = s_transparent_mount->pwrite(s_transparent_mount,
						      handle, buf, count, offset);
    if ( bytes_wrote >= 0 ){
	/*get wrote bytes by pointer*/
	*nwrote = bytes_wrote;
	ret=0;
    }
    LOG_INFO_SYSCALL_FINISH( ret, "bytes_wrote=%d, handle=%d count=%u", 
			     bytes_wrote, handle, count);
    if ( log_state )
	__zrt_log_enable(log_state); /*restore log state*/
    return ret;
}
////////////////

int  zrt_zcall_enhanced_seek(int handle, off_t offset, int whence, off_t *new_offset){
    int ret=-1;
    LOG_SYSCALL_START("handle=%d offset=%lld whence=%s", handle, offset, STR_SEEK_WHENCE(whence));
    errno = 0;

    if ( whence == SEEK_SET && offset < 0 ){
	SET_ERRNO(EINVAL);
    }
    else{
	offset = s_transparent_mount->lseek(s_transparent_mount,handle, offset, whence);
	if ( offset != -1 ){
	    /*get new offset by pointer*/
	    *new_offset = offset;
	    ret=0;
	}
	else{
	    /*return errno as error*/
	    return errno;
	}
    }

    LOG_SHORT_SYSCALL_FINISH( ret, "newoffset=%lld, handle=%d whence=%s", 
			      offset, handle, STR_SEEK_WHENCE(whence));
    return ret;
}

int  zrt_zcall_enhanced_fstat(int handle, struct stat *st){
    LOG_SYSCALL_START("handle=%d stat=%p", handle, st);
    errno = 0;
    VALIDATE_SYSCALL_PTR(stat);

    int ret = s_transparent_mount->fstat(s_transparent_mount, handle, st);
    if ( ret == 0 ){
	ZRT_LOG_STAT(L_SHORT, st);
    }
    LOG_SHORT_SYSCALL_FINISH( ret, "handle=%d", handle);
    return ret;
}

int  zrt_zcall_enhanced_getdents(int fd, struct dirent *dirent_buf, size_t count, size_t *nread){
    int ret = -1;
    LOG_SYSCALL_START("fd=%d dirent_buf=%p count=%u", fd, dirent_buf, count);
    errno=0;
    VALIDATE_SYSCALL_PTR(dirent_buf);

    int32_t bytes_readed = s_transparent_mount->getdents(s_transparent_mount, fd, (char*)dirent_buf, count);
    if ( bytes_readed >= 0 ){
	*nread = bytes_readed;
	ret=0;
    }
    LOG_SHORT_SYSCALL_FINISH( ret, "fd=%d count=%u", fd, count);
    return ret;
}

int  zrt_zcall_enhanced_open(const char *name, int flags, mode_t mode, int *newfd){
    LOG_SYSCALL_START("name=%s flags=%d mode=%o(octal)", name, flags, mode );

    int ret=-1;
    errno=0;
    VALIDATE_SYSCALL_PTR(name);
    
    /*reset mode bits, that is not actual for permissions*/
    mode&=(S_IRWXU|S_IRWXG|S_IRWXO);
    APPLY_UMASK(&mode);

    if ( (ret = s_transparent_mount->open(s_transparent_mount, name, flags, mode )) >= 0 ){
	/*get fd by pointer*/
	*newfd  = ret;
	ret =0;
    }

    LOG_SHORT_SYSCALL_FINISH( ret, 
			      "*newfd=%d, name=%s, flags=%s, mode=%s", 
			      *newfd, name, 
			      STR_ALLOCA_COPY(STR_FILE_OPEN_FLAGS(flags)),
			      STR_ALLOCA_COPY(STR_STAT_ST_MODE(mode)));
    return ret;
}

int  zrt_zcall_enhanced_stat(const char *pathname, struct stat * stat){
    LOG_SYSCALL_START("pathname=%s stat=%p", pathname, stat);
    int ret=-1;
    errno = 0;
    VALIDATE_SYSCALL_PTR(pathname);
    VALIDATE_SYSCALL_PTR(stat);

    if ( (ret = s_transparent_mount->stat(s_transparent_mount, pathname, stat)) == 0 ){
	ZRT_LOG_STAT(L_SHORT, stat);
    }

    LOG_SHORT_SYSCALL_FINISH( ret, "pathname=%s", pathname);
    return ret;
}

/*
 * following 3 functions (sysbrk, mmap, munmap) is the part of the
 * new memory engine. the new allocate specified amount of ram before
 * user code start and then user can only obtain that allocated memory.
 */

/* irt memory *************************/
int  zrt_zcall_enhanced_sysbrk(void **newbrk){
    int ret=-1;
    LOG_SYSCALL_START("*newbrk=%p", *newbrk);
    struct MemoryManagerPublicInterface* memif = memory_interface_instance();
    int32_t retaddr = memif->sysbrk(memif, *newbrk );
    if ( retaddr != -1 ){
	/*get new address via pointer*/
	*newbrk = (void*)retaddr;
	ret=0;
    }
    LOG_INFO_SYSCALL_FINISH( retaddr, "*newbrk=%p", *newbrk);
    return ret;
}

int  zrt_zcall_enhanced_mmap(void **addr, size_t length, int prot, int flags, int fd, off_t off){
    int32_t retcode = -1;
    LOG_SYSCALL_START("addr=%p length=%u prot=%u flags=%u fd=%u off=%lld",
    		      *addr, length, prot, flags, fd, off);

    struct MemoryManagerPublicInterface* memif = memory_interface_instance();
    retcode = memif->mmap(memif, *addr, length, prot,
				       flags, fd, off);
    if ( (void*)retcode != MAP_FAILED ){
	*addr = (void*)retcode;
	retcode=0;
    }
  
    LOG_INFO_SYSCALL_FINISH( retcode,
			     "addr=%p length=%u prot=%s flags=%s fd=%u off=%lld",
			     *addr, length, 
			     STR_ALLOCA_COPY(STR_MMAP_PROT_FLAGS(prot)), 
			     STR_ALLOCA_COPY(STR_MMAP_FLAGS(flags)),
			     fd, off);
    return retcode;
}

int  zrt_zcall_enhanced_munmap(void *addr, size_t len){
    LOG_SYSCALL_START("addr=%p, len=%u", addr, len);
    struct MemoryManagerPublicInterface* memif = memory_interface_instance();
    int32_t retcode = memif->munmap(memif, addr, len);
    LOG_INFO_SYSCALL_FINISH( retcode, "addr=%p, len=%u", addr, len);
    return retcode;
}

/***********************************************************
 *ZRT initializators
 ***********************************************************/

void zrt_internal_session_info( const struct UserManifest const* manifest ){
    int i;
    char **envp = environ;
    time_t t = time(NULL);
    int pages_count = (int)(manifest->heap_size / sysconf(_SC_PAGE_SIZE));

    LOG_DEBUG(ELogTitle, "ZVM SESSION INFO", "=======");
    LOG_DEBUG(ELogAddress, (intptr_t)manifest->heap_ptr, "ZVM Manifest heap pointer" );
    LOG_DEBUG(ELogSize, manifest->heap_size, "ZVM Manifest heap size" );
    
    /*get from system, print environment variables*/
    LOG_DEBUG(ELogTime, ctime(&t), "System time" );
    LOG_DEBUG(ELogSize, sysconf(_SC_PAGE_SIZE), "Page size _SC_PAGE_SIZE" );
    LOG_DEBUG(ELogSize, sysconf(_SC_PHYS_PAGES), "sysconf(_SC_PHYS_PAGES)" )
    LOG_DEBUG(ELogSize, sysconf(_SC_AVPHYS_PAGES), "sysconf(_SC_AVPHYS_PAGES)" )
    LOG_DEBUG(ELogCount, pages_count, "Memory pages count"  );
    LOG_DEBUG(ELogAddress, manifest->heap_ptr + manifest->heap_size, "Heap highest page" );
    LOG_DEBUG(ELogAddress, sbrk(0), "sbrk(0)" );
    LOG_DEBUG(ELogPath, get_current_dir_name(), "Home directory" );

    LOG_DEBUG(ELogTitle, "Environment variables", "======");
    i=0;
    while( envp[i] ){
        ZRT_LOG(L_BASE, "envp[%d] = '%s'", i, envp[i]);
	++i;
    }

    LOG_DEBUG(ELogCount, manifest->channels_count, "channels list");
    /*print channels list*/
    for(i = 0; i < manifest->channels_count; ++i)
	{
	    ZRT_LOG(L_BASE, "channel[%2d].name = '%s'", i, manifest->channels[i].name);
	    ZRT_LOG(L_BASE, "channel[%2d].type=%d, size=%lld", i, 
		    manifest->channels[i].type, manifest->channels[i].size);
	    ZRT_LOG(L_BASE, "channel[%2d].limits[GetsLimit=%7lld, GetSizeLimit=%7lld]", i, 
		    manifest->channels[i].limits[GetsLimit], 
		    manifest->channels[i].limits[GetSizeLimit]);
	    ZRT_LOG(L_BASE, "channel[%2d].limits[PutsLimit=%7lld, PutSizeLimit=%7lld]", i, 
		    manifest->channels[i].limits[PutsLimit],
		    manifest->channels[i].limits[PutSizeLimit]);
	}
}

/*Basic zrt initializer.  The argv & envp vectors will be set after
this function will return a value. If precache requested by nvram,
then fork will be called here, to be able provide both args & envs
from new nvram file for forked session. Thus sections [args], [env]
should not be specified in initial nvram file if precache is
enabled.*/
void zrt_zcall_enhanced_zrt_setup(void){
    struct NvramLoaderPublicInterface* nvram = INSTANCE_L(NVRAM_LOADER)();
    zrt_internal_init(MANIFEST);

    if ( NULL != nvram->section_by_name( nvram, MAPPING_SECTION_NAME ) ){
	nvram->handle(nvram, HANDLE_ONLY_MAPPING_SECTION, NULL, NULL, NULL);
    }
    if ( NULL != nvram->section_by_name( nvram, FSTAB_SECTION_NAME ) ){
	nvram->handle(nvram, (struct MNvramObserver*)HANDLE_ONLY_FSTAB_SECTION, 
		      s_channels_mount, s_transparent_mount, NULL );
    }
    /*check nvram section [precache] and call fork if needed*/
    if ( NULL != nvram->section_by_name( nvram, PRECACHE_SECTION_NAME ) ){
	int dofork = 0;
	/*return result via dofork pointer*/
	nvram->handle(nvram, HANDLE_ONLY_PRECACHE_SECTION, &dofork, NULL, NULL);
	if ( dofork ){
	    zfork();
	}
    }
}


void zrt_zcall_enhanced_premain(void){
    set_home_dir( getenv("HOME") );
    zrt_internal_session_info(MANIFEST);
    ZRT_LOG(L_SHORT, P_TEXT, "user main() begin");
    ZRT_LOG_DELIMETER;
}

void zrt_zcall_enhanced_postmain(int usercode){
    ZRT_LOG(L_SHORT, P_TEXT, "user main() end");
    ZRT_LOG_DELIMETER;
}


/*1st step zrt initializer*/
void zrt_internal_init( const struct UserManifest const* manifest ){
    ITEMS_CREATOR;

    /*construct memory manager and pass zerovm MANIFEST members:
      (1)heap start address, (2)heap size, (3)existing brk address got
      from zcalls_prolog interface*/
    CONSTRUCT_L(MEMORY_MANAGER) ( manifest->heap_ptr, 
				  manifest->heap_size, 
				  static_prolog_brk() );

    /*manage mounted filesystems*/
    s_mounts_manager = get_mounts_manager();

    /*alloc filesystem based on channels*/
    struct ChannelsModeUpdaterPublicInterface *nvram_mode_setting_updater;

    s_channels_mount = 
	CONSTRUCT_L(CHANNELS_FILESYSTEM)( &nvram_mode_setting_updater,
					  s_mounts_manager->handle_allocator,
					  s_mounts_manager->open_files_pool,
					  manifest->channels, 
					  manifest->channels_count,
					  s_emu_channels, 
					  sizeof(s_emu_channels)/sizeof(struct ZVMChannel));
    set_mapping_channels_settings_updater( nvram_mode_setting_updater ); 

    /*alloc main filesystem that combines all filesystems mounts*/
    s_transparent_mount = alloc_transparent_mount( s_mounts_manager );

    s_zrt_ready = 1;

    /*open standard files to conform C*/
    s_channels_mount->open( s_channels_mount, DEV_STDIN, O_RDONLY, 0 );
    s_channels_mount->open( s_channels_mount, DEV_STDOUT, O_WRONLY, 0 );
    s_channels_mount->open( s_channels_mount, DEV_STDERR, O_WRONLY, 0 );

    /*create mem mount*/
    s_mem_mount = CONSTRUCT_L(INMEMORY_FILESYSTEM)( s_mounts_manager->handle_allocator,
						    s_mounts_manager->open_files_pool);

    /*Mount filesystems*/
    s_mounts_manager->mount_add( "/dev", s_channels_mount );
    s_mounts_manager->mount_add( "/", s_mem_mount );

    /*explicitly create /dev directory in memmount, it's required for consistent
      FS structure, readdir from now can list /dev dir recursively from root */
    s_mem_mount->mkdir( s_mem_mount, "/dev", 0777 );
    /*add directories here that can be expected by some user applications */
    s_mem_mount->mkdir( s_mem_mount, "/tmp", 0777 );

    /*user main execution just after zrt initialization*/
}

int is_zrt_ready(){
    return s_zrt_ready;
}


int zfork(){
    ZRT_LOG(L_INFO, P_TEXT, "call zvm_fork");
    /*zvm fork syscall here
      ...*/
    int res = zvm_fork();
    ZRT_LOG(L_INFO, "zvm_fork res=%d", res);

    /*re-read nvram file because after fork his content can be changed. */
    /*Use updated nvram fields that we get with forked session*/
    /*Folowing nvram handlers using only stack and nor heap*/
    struct NvramLoaderPublicInterface* nvram = INSTANCE_L(NVRAM_LOADER)();
    /*if nvram config file not empty then do parsing*/
    //    if ( nvram->read(nvram, "/dev/nvram2") > 0 ){
    if ( nvram->read(nvram, DEV_NVRAM) > 0 ){
	nvram->parse(nvram);

	/*all sections must be handled here except [precache, args] at fork*/

	/*[env] section*/
	if ( NULL != nvram->section_by_name( nvram, ENVIRONMENT_SECTION_NAME ) ){
	    nvram->handle(nvram, HANDLE_ONLY_ENV_SECTION, NULL, NULL, NULL);
	}
	/*[mapping] section*/
	if ( NULL != nvram->section_by_name( nvram, MAPPING_SECTION_NAME ) ){
	    nvram->handle(nvram, HANDLE_ONLY_MAPPING_SECTION, NULL, NULL, NULL);
	}
	/*[fstab] section*/
	if ( NULL != nvram->section_by_name( nvram, FSTAB_SECTION_NAME ) ){
	    /*remove existing fstab records*/
	    get_fstab_observer()->erase_old_mounts(HANDLE_ONLY_FSTAB_SECTION);
	    nvram->handle(nvram, (struct MNvramObserver*)HANDLE_ONLY_FSTAB_SECTION, 
			  s_channels_mount, s_transparent_mount, NULL );
	    /*update state for removable mounts, all removable mounts needs to be refreshed*/
	    get_fstab_observer()->reset_removable(HANDLE_ONLY_FSTAB_SECTION);

	}
	/*[debug] section - verbosity*/
	if ( NULL != nvram->section_by_name( nvram, DEBUG_SECTION_NAME ) ){
	    ZRT_LOG(L_INFO, "%s", "nvram handle debug");
	    nvram->handle(nvram, HANDLE_ONLY_DEBUG_SECTION, NULL, NULL, NULL );
	}
    }
    ZRT_LOG(L_SHORT, "zfork() res=%d ", res);
    return res;
}

/*************************************************************************/




