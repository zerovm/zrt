/*
 * transparent_mount.c
 *
 *  Created on: 19.09.2012
 *      Author: yaroslav
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

static struct MountsManager* s_mounts_manager;

#define CONVERT_PATH_TO_MOUNT(full_path)			\
    s_mounts_manager->convert_path_to_mount( full_path )

static int transparent_chown(const char* path, uid_t owner, gid_t group){
    struct MountsInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->chown( CONVERT_PATH_TO_MOUNT(path), owner, group);
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_chmod(const char* path, uint32_t mode){
    struct MountsInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->chmod( CONVERT_PATH_TO_MOUNT(path), mode);
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_stat(const char* path, struct stat *buf){
    struct MountsInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
            return mount->stat( CONVERT_PATH_TO_MOUNT(path), buf);
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_mkdir(const char* path, uint32_t mode){
    struct MountsInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->mkdir( CONVERT_PATH_TO_MOUNT(path), mode);
    else{
	SET_ERRNO(ENOENT);
        return -1;
    }
}

static int transparent_rmdir(const char* path){
    struct MountsInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->rmdir( CONVERT_PATH_TO_MOUNT(path) );
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_umount(const char* path){
    SET_ERRNO(ENOSYS);
    return -1;
}

static int transparent_mount(const char* path, void *mount_){
    SET_ERRNO(ENOSYS);
    return -1;
}

static ssize_t transparent_read(int fd, void *buf, size_t nbyte){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->read(fd, buf, nbyte);
    else{
        SET_ERRNO(EBADF);
        return -1;
    }
}

static ssize_t transparent_write(int fd, const void *buf, size_t nbyte){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->write(fd, buf, nbyte);
    else{
        SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_fchown(int fd, uid_t owner, gid_t group){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->fchown(fd, owner, group);
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_fchmod(int fd, mode_t mode){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->fchmod(fd, mode);
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_fstat(int fd, struct stat *buf){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount ){
        return mount->fstat(fd, buf);
    }
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_getdents(int fd, void *buf, unsigned int count){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->getdents(fd, buf, count);
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_fsync(int fd){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->fsync(fd);
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_close(int fd){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->close(fd);
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static off_t transparent_lseek(int fd, off_t offset, int whence){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount ){
        struct stat st;
        int ret = mount->fstat(fd, &st );
        if ( ret != 0 ) return -1; //errno sould be set by mount->fstat
        if ( S_ISDIR(st.st_mode) ){
	    SET_ERRNO(EBADF);
            return -1;
        }
        return mount->lseek(fd, offset, whence);
    }
    else{
	SET_ERRNO(EBADF);
        return -1;
    }
}

static int transparent_open(const char* path, int oflag, uint32_t mode){
    struct MountsInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->open( CONVERT_PATH_TO_MOUNT(path), oflag, mode );
    else{
	SET_ERRNO(ENOENT);
        return -1;
    }
}

static int transparent_fcntl(int fd, int cmd, ...){
    int ret=0;
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount ){
	va_list args;
	va_start(args, cmd);
	if ( cmd == F_SETLK || cmd == F_SETLKW || cmd == F_GETLK ){
	    struct flock* input_lock = va_arg(args, struct flock*);
	    ZRT_LOG(L_SHORT, "flock=%p", input_lock );
	    ret = mount->fcntl( fd, cmd, input_lock );
	}
	else{
	    ret=-1;
	    SET_ERRNO(ENOSYS);
	}
	va_end(args);
    }
    else{
        SET_ERRNO(ENOENT);
        ret = -1;
    }
    return ret;
}


static int transparent_remove(const char* path){
    struct MountsInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->remove( CONVERT_PATH_TO_MOUNT(path) );
    else{
        SET_ERRNO(ENOENT);
        return -1;
    }
}

static int transparent_unlink(const char* path){
    struct MountsInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->unlink( CONVERT_PATH_TO_MOUNT(path) );
    else{
        SET_ERRNO(ENOENT);
        return -1;
    }
}

static int transparent_access(const char* path, int amode){
    struct MountsInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->access( CONVERT_PATH_TO_MOUNT(path), amode );
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_ftruncate_size(int fd, off_t length){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
	return mount->ftruncate_size( fd, length );
    else{
	SET_ERRNO( EBADF );
        return -1;
    }
}

static int transparent_truncate_size(const char* path, off_t length){
    struct MountsInterface* mount = s_mounts_manager->mount_bypath(path); 
    if ( mount )
	return mount->truncate_size( CONVERT_PATH_TO_MOUNT(path), length );
    else{
	SET_ERRNO( EBADF );
        return -1;
    }
}


static int transparent_isatty(int fd){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->isatty(fd);
    else{
        errno = EBADF;
        return -1;
    }
}

static int transparent_dup(int oldfd){
    SET_ERRNO(ENOSYS);
    return -1;
}

static int transparent_dup2(int oldfd, int newfd){
    SET_ERRNO(ENOSYS);
    return -1;
}

static int transparent_link(const char *oldpath, const char *newpath){
    struct MountsInterface* mount1 = s_mounts_manager->mount_bypath(oldpath); 
    struct MountsInterface* mount2 = s_mounts_manager->mount_bypath(newpath); 
    if ( mount1 == mount2 && mount1 != NULL ){
	return mount1->link(CONVERT_PATH_TO_MOUNT(oldpath),
			    CONVERT_PATH_TO_MOUNT(newpath) );
    }
    else{
        SET_ERRNO(ENOENT);
        return -1;
    }
}


static struct MountsInterface s_transparent_mount = {
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

struct MountsInterface* alloc_transparent_mount( struct MountsManager* mounts_manager ){
    s_mounts_manager = mounts_manager;
    return &s_transparent_mount;
}


