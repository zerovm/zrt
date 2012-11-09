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
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "mounts_manager.h"
#include "mounts_interface.h"

static struct MountsManager* s_mounts_manager;


static int transparent_chown(const char* path, uid_t owner, gid_t group){
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        if ( mount_info->mount->mount_id == EChannelsMountId ) /*for channels mount do not use path transformation*/
            return mount_info->mount->chown( path, owner, group);
        else{
            return mount_info->mount->chown( s_mounts_manager->get_nested_mount_path( mount_info, path ), owner, group);
        }
    }
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_chmod(const char* path, uint32_t mode){
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        if ( mount_info->mount->mount_id == EChannelsMountId ) /*for channels mount do not use path transformation*/
            return mount_info->mount->chmod( path, mode);
        else{
            return mount_info->mount->chmod( s_mounts_manager->get_nested_mount_path( mount_info, path ), mode);
        }
    }
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_stat(const char* path, struct stat *buf){
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        int ret;
        if ( mount_info->mount->mount_id == EChannelsMountId ) /*for channels mount do not use path transformation*/
            ret = mount_info->mount->stat( path, buf);
        else{
            ret = mount_info->mount->stat( s_mounts_manager->get_nested_mount_path( mount_info, path ), buf);
        }
        return ret;
    }
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_mkdir(const char* path, uint32_t mode){
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        if ( mount_info->mount->mount_id == EChannelsMountId ) /*for channels mount do not use path transformation*/
            return mount_info->mount->mkdir( path, mode);
        else{
            return mount_info->mount->mkdir( s_mounts_manager->get_nested_mount_path( mount_info, path ), mode);
        }
    }
    else{
	SET_ERRNO(ENOENT);
        return -1;
    }
}

static int transparent_rmdir(const char* path){
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        if ( mount_info->mount->mount_id == EChannelsMountId ) /*for channels mount do not use path transformation*/
            return mount_info->mount->rmdir( path );
        else{
            return mount_info->mount->rmdir( s_mounts_manager->get_nested_mount_path( mount_info, path ) );
        }
    }
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_umount(const char* path){
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        if ( mount_info->mount->mount_id == EChannelsMountId ) /*for channels mount do not use path transformation*/
            return mount_info->mount->umount( path );
        else{
            return mount_info->mount->umount( s_mounts_manager->get_nested_mount_path( mount_info, path ) );
        }
    }
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_mount(const char* path, void *mount_){
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        if ( mount_info->mount->mount_id == EChannelsMountId ) /*for channels mount do not use path transformation*/
            return mount_info->mount->mount( path, mount_ );
        else{
            return mount_info->mount->mount( s_mounts_manager->get_nested_mount_path( mount_info, path ), mount_ );
        }
    }
    else{
        errno = ENOENT;
        return -1;
    }
}

static ssize_t transparent_read(int fd, void *buf, size_t nbyte){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->read(fd, buf, nbyte);
    else{
        errno = EBADF;
        return -1;
    }
}

static ssize_t transparent_write(int fd, const void *buf, size_t nbyte){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->write(fd, buf, nbyte);
    else{
        errno = EBADF;
        return -1;
    }
}

static int transparent_fchown(int fd, uid_t owner, gid_t group){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->fchown(fd, owner, group);
    else{
        errno = EBADF;
        return -1;
    }
}

static int transparent_fchmod(int fd, mode_t mode){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->fchmod(fd, mode);
    else{
        errno = EBADF;
        return -1;
    }
}

static int transparent_fstat(int fd, struct stat *buf){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount ){
        return mount->fstat(fd, buf);
    }
    else{
        errno = EBADF;
        return -1;
    }
}

static int transparent_getdents(int fd, void *buf, unsigned int count){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->getdents(fd, buf, count);
    else{
        errno = EBADF;
        return -1;
    }
}

static int transparent_fsync(int fd){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->fsync(fd);
    else{
        errno = EBADF;
        return -1;
    }
}

static int transparent_close(int fd){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount )
        return mount->close(fd);
    else{
        errno = EBADF;
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
            errno=EBADF;
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
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        if ( mount_info->mount->mount_id == EChannelsMountId ) {
	    /*for channels mount do not use path transformation*/
            return mount_info->mount->open( path, oflag, mode );
	}
        else{
            return mount_info->mount
		->open( s_mounts_manager->get_nested_mount_path( mount_info, path ), 
			oflag, mode );
        }
    }
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_fcntl(int fd, int cmd, ...){
    struct MountsInterface* mount = s_mounts_manager->mount_byhandle(fd);
    if ( mount ){
	va_list args;
	va_start(args, cmd);
	int retcode = mount->fcntl( fd, cmd, args );
	va_end(args);
	return retcode;
    }
    else{
        errno = ENOENT;
        return -1;
    }
}


static int transparent_remove(const char* path){
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        if ( mount_info->mount->mount_id == EChannelsMountId ) /*for channels mount do not use path transformation*/
            return mount_info->mount->remove( path );
        else{
            return mount_info->mount->remove( s_mounts_manager->get_nested_mount_path( mount_info, path ) );
        }
    }
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_unlink(const char* path){
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        if ( mount_info->mount->mount_id == EChannelsMountId ) /*for channels mount do not use path transformation*/
            return mount_info->mount->unlink( path );
        else{
            return mount_info->mount->unlink( s_mounts_manager->get_nested_mount_path( mount_info, path ) );
        }
    }
    else{
        errno = ENOENT;
        return -1;
    }
}

static int transparent_access(const char* path, int amode){
    struct MountInfo* mount_info = s_mounts_manager->mountinfo_bypath(path);
    if ( mount_info ){
        if ( mount_info->mount->mount_id == EChannelsMountId ) /*for channels mount do not use path transformation*/
            return mount_info->mount->access( path, amode );
        else{
            return mount_info->mount->access( s_mounts_manager->get_nested_mount_path( mount_info, path ), amode );
        }
    }
    else{
        errno = ENOENT;
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

static int transparent_link(const char* path1, const char* path2){
    SET_ERRNO(ENOSYS);
    return -1;
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
        transparent_isatty,
        transparent_dup,
        transparent_dup2,
        transparent_link
};

struct MountsInterface* alloc_transparent_mount( struct MountsManager* mounts_manager ){
    s_mounts_manager = mounts_manager;
    return &s_transparent_mount;
}


