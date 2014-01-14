/*
 * Filesystem interface implementation. 
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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "mounts_manager.h"
#include "mounts_interface.h"
#include "fcntl_implem.h"

static struct MountsManager* s_mounts_manager;

#define CONVERT_PATH_TO_MOUNT(full_path)			\
    s_mounts_manager->convert_path_to_mount( full_path )

static int transparent_chown(struct MountsPublicInterface *this, 
			     const char* path, uid_t owner, gid_t group){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->chown( mount, CONVERT_PATH_TO_MOUNT(path), owner, group);
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_chmod(struct MountsPublicInterface *this,
			     const char* path, uint32_t mode){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->chmod( mount, CONVERT_PATH_TO_MOUNT(path), mode);
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_stat(struct MountsPublicInterface *this,
			    const char* path, struct stat *buf){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->stat( mount, CONVERT_PATH_TO_MOUNT(path), buf);
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_mkdir(struct MountsPublicInterface *this,
			     const char* path, uint32_t mode){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->mkdir( mount, CONVERT_PATH_TO_MOUNT(path), mode);
    else{
	SET_ERRNO(ENOENT);
        return -1;
    }
}

static int transparent_rmdir(struct MountsPublicInterface *this,
			     const char* path){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->rmdir( mount, CONVERT_PATH_TO_MOUNT(path) );
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_umount(struct MountsPublicInterface *this, const char* path){
    SET_ERRNO(ENOSYS);
    return -1;
}

static int transparent_mount(struct MountsPublicInterface *this, 
			     const char* path, void *mount_){
    SET_ERRNO(ENOSYS);
    return -1;
}

static ssize_t transparent_read(struct MountsPublicInterface *this,
				int fd, void *buf, size_t nbyte){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->read( mount, fd, buf, nbyte);
    else{
        SET_ERRNO(EBADF);
        return -1;
    }
}

static ssize_t transparent_write(struct MountsPublicInterface *this,
				 int fd, const void *buf, size_t nbyte){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->write( mount, fd, buf, nbyte);
    else{
        SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_fchown(struct MountsPublicInterface *this,
			      int fd, uid_t owner, gid_t group){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->fchown( mount, fd, owner, group);
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_fchmod(struct MountsPublicInterface *this,
			      int fd, mode_t mode){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->fchmod( mount, fd, mode);
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_fstat(struct MountsPublicInterface *this,
			     int fd, struct stat *buf){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount ){
        return mount->fstat( mount, fd, buf);
    }
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_getdents(struct MountsPublicInterface *this,
				int fd, void *buf, unsigned int count){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->getdents( mount, fd, buf, count);
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_fsync(struct MountsPublicInterface *this, int fd){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->fsync(mount, fd);
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_close(struct MountsPublicInterface *this, int fd){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->close(mount, fd);
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static off_t transparent_lseek(struct MountsPublicInterface *this,
			       int fd, off_t offset, int whence){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount ){
        struct stat st;
        int ret = mount->fstat(mount, fd, &st );
        if ( ret != 0 ) return -1; //errno sould be set by mount->fstat
        if ( S_ISDIR(st.st_mode) ){
	    SET_ERRNO(EBADF);
            return -1;
        }
        return mount->lseek(mount, fd, offset, whence);
    }
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_open(struct MountsPublicInterface *this,
			    const char* path, int oflag, uint32_t mode){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->open( mount, CONVERT_PATH_TO_MOUNT(path), oflag, mode );
    else{
	SET_ERRNO(ENOENT);
        return -1;
    }
}

static int transparent_fcntl(struct MountsPublicInterface *this,
			     int fd, int cmd, ...){
    int ret=0;
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount ){
	va_list args;
	/*operating with file locks*/
	if ( cmd == F_SETLK || cmd == F_SETLKW || cmd == F_GETLK ){
	    va_start(args, cmd);
	    struct flock* input_lock = va_arg(args, struct flock*);
	    ZRT_LOG(L_SHORT, "flock=%p", input_lock );
	    if ( 0 == (ret=mount->fcntl(mount, fd, cmd, input_lock)) ){
		ret = fcntl_implem(mount->implem(mount), fd, cmd, input_lock);
	    }
	    va_end(args);
	}
	/*get file status flags, like flags specified with open function*/
	else if( cmd == F_GETFL	){
	    if ( 0 == (ret=mount->fcntl(mount, fd, cmd)) ){
		ret = fcntl_implem(mount->implem(mount), fd, cmd);
	    }
	}
	/*set file status flags, like flags specified with open function*/
	else if( cmd == F_SETFL	){
	    va_start(args, cmd);
	    long flags = va_arg(args, long);
	    ZRT_LOG(L_SHORT, "F_SETFL flags=%ld", flags );

	    if ( 0 == (ret=mount->fcntl(mount, fd, cmd, flags)) ){
		ret = fcntl_implem(mount->implem(mount), fd, cmd, flags);
	    }
	}
	else{
	    ret=-1;
	    SET_ERRNO(ENOSYS);
	}
    }
    else{
        SET_ERRNO(EBADF);
        ret = -1;
    }
    return ret;
}


static int transparent_remove(struct MountsPublicInterface *this,
			      const char* path){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->remove( mount, CONVERT_PATH_TO_MOUNT(path) );
    else{
        SET_ERRNO(ENOENT);
        return -1;
    }
}

static int transparent_unlink(struct MountsPublicInterface *this,
			      const char* path){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->unlink( mount, CONVERT_PATH_TO_MOUNT(path) );
    else{
        SET_ERRNO(ENOENT);
        return -1;
    }
}

static int transparent_access(struct MountsPublicInterface *this,
			      const char* path, int amode){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->access( mount, CONVERT_PATH_TO_MOUNT(path), amode );
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_ftruncate_size(struct MountsPublicInterface *this,
				      int fd, off_t length){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
	return mount->ftruncate_size( mount, fd, length );
    else{
	SET_ERRNO( EBADF );
        return -1;
    }
}

static int transparent_truncate_size(struct MountsPublicInterface *this,
				     const char* path, off_t length){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->truncate_size( mount, CONVERT_PATH_TO_MOUNT(path), length );
    else{
	SET_ERRNO( EBADF );
        return -1;
    }
}


static int transparent_isatty(struct MountsPublicInterface *this, int fd){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->isatty(mount, fd);
    else{
        errno = EBADF;
        return -1;
    }
}

static int transparent_dup(struct MountsPublicInterface *this, int oldfd){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(oldfd);
    if ( mount )
	return mount->dup( mount, oldfd );
    else{
	SET_ERRNO( EBADF );
        return -1;
    }
}

static int transparent_dup2(struct MountsPublicInterface *this, int oldfd, int newfd){
    struct MountsPublicInterface* mount = s_mounts_manager->mount_byhandle(oldfd);
    if ( mount )
	return mount->dup2( mount, oldfd, newfd );
    else{
	SET_ERRNO( EBADF );
        return -1;
    }
}

static int transparent_link(struct MountsPublicInterface *this,
			    const char *oldpath, const char *newpath){
    struct MountsPublicInterface* mount1 = s_mounts_manager->mount_bypath(oldpath); 
    struct MountsPublicInterface* mount2 = s_mounts_manager->mount_bypath(newpath); 
    if ( mount1 == mount2 && mount1 != NULL ){
	return mount1->link(mount1, CONVERT_PATH_TO_MOUNT(oldpath),
			    CONVERT_PATH_TO_MOUNT(newpath) );
    }
    else{
        SET_ERRNO(ENOENT);
        return -1;
    }
}


static struct MountsPublicInterface s_transparent_mount = {
        transparent_chown,
        transparent_chmod,
        transparent_stat,
        transparent_mkdir,
        transparent_rmdir,
        transparent_umount,
        transparent_mount,
        transparent_read,
        transparent_write,
        transparent_fchown,
        transparent_fchmod,
        transparent_fstat,
        transparent_getdents,
        transparent_fsync,
        transparent_close,
        transparent_lseek,
        transparent_open,
	transparent_fcntl,
        transparent_remove,
        transparent_unlink,
        transparent_access,
	transparent_ftruncate_size,
	transparent_truncate_size,
        transparent_isatty,
        transparent_dup,
        transparent_dup2,
        transparent_link
};

struct MountsPublicInterface* alloc_transparent_mount( struct MountsManager* mounts_manager ){
    s_mounts_manager = mounts_manager;
    return &s_transparent_mount;
}


