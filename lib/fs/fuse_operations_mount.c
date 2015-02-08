/*
 *
 * Copyright (c) 2015, Rackspace, Inc.
 * Author: Yaroslav Litvinov, yaroslav.litvinov@rackspace.com
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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h"

#include "fuse_operations_mount.h"

#include <fuse.h>
#include <fs/mounts_interface.h>
#include <helpers/path_utils.h>
#include "handle_allocator.h" //struct HandleAllocator, struct HandleItem
#include "open_file_description.h" //struct OpenFilesPool, struct OpenFileDescription
#include "dirent_engine.h"
#include "enum_strings.h"

#define GET_STAT_HANDLE_CHECK(fs, fd, stat_p)	\
    entry_p = (fs)->handle_allocator->entry( (fd) );	\
    if ( entry == NULL ){				\
	SET_ERRNO(EBADF);				\
	return -1;					\
    }

#define GET_STAT_ENSURE_EXIST(fs, path, stat_p)				\
    if ( (fs)->fuse_operations->getattr == NULL ){			\
	SET_ERRNO(ENOSYS);						\
	return -1;							\
    }									\
    else if ( (fs)->fuse_operations->getattr(path, stat_p) != 0 ){	\
	SET_ERRNO(ENOENT);						\
	return -1;							\
    }									\

#define GET_PARENT_STAT_ENSURE_EXIST(fs, path, stat_p){			\
	int cursor_path;						\
	int parent_dir_len;						\
	INIT_TEMP_CURSOR(&cursor_path);					\
	const char *parent_dir = path_subpath_backward(&cursor_path, path, &parent_dir_len); \
	if ( parent_dir != NULL &&					\
	     (fs)->fuse_operations->getattr(parent_dir, stat_p ) != 0 && \
	     S_ISDIR((stat_p)->st_mode) ){				\
	    ZRT_LOG(L_ERROR, "%s: %s parent_dir can't be obtained", __FILE__, path); \
	    SET_ERRNO(ENOTDIR);						\
	    return -1;							\
	}								\
    }

#define CHECK_FUNC_ENSURE_EXIST(fs, funcname)	\
    if ( (fs)->fuse_operations->funcname == NULL ){	\
	SET_ERRNO(ENOSYS);			\
	return -1;				\
    }

struct FuseFileOptionalData
{
    char *path; /*for fuse functions that needs a path*/
    /*fuse_file_info holds the fuse data. Particularly it has a file
     handle which is local for one fuse filesystem and shouldn't be
     used outside of fuse. So every fuse handle must have
     corresponding handle issued by zrt.*/
    struct fuse_file_info *finfo;
};

struct FuseOperationsMount{
    struct MountsPublicInterface public_;
    struct HandleAllocator* handle_allocator;
    struct OpenFilesPool*   open_files_pool;
    struct fuse_operations* fuse_operations;
    struct MountSpecificPublicInterface* mount_specific_interface;
};


/*wraper implementation
 All checks on zfs top level will be skipped, because it does in lowlevel*/

static ssize_t fuse_operations_mount_readlink(struct MountsPublicInterface* this_,
				 const char *path, char *buf, size_t bufsize){
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    int ret = -1;

    CHECK_FUNC_ENSURE_EXIST(fs, readlink);

    if ( (ret=fs->fuse_operations->readlink(path, buf, bufsize)) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }
    
    return ret;
}

static int fuse_operations_mount_symlink(struct MountsPublicInterface* this_,
			    const char *oldpath, const char *newpath){
    int ret=-1;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;

    CHECK_FUNC_ENSURE_EXIST(fs, symlink);

    if ( (ret=fs->fuse_operations->symlink(oldpath, newpath)) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}



static int fuse_operations_mount_chown(struct MountsPublicInterface* this_, const char* path, uid_t owner, gid_t group){
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    int ret = -1;

    CHECK_FUNC_ENSURE_EXIST(fs, chown);

    if ( (ret=fs->fuse_operations->chown(path, owner, group)) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }
    
    return ret;
}

static int fuse_operations_mount_chmod(struct MountsPublicInterface* this_, const char* path, uint32_t mode){
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    int ret = -1;

    CHECK_FUNC_ENSURE_EXIST(fs, chmod);

    if ( (ret=fs->fuse_operations->chmod(path, mode)) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}

static int fuse_operations_mount_statvfs(struct MountsPublicInterface* this_, const char* path, struct statvfs *buf){
    int ret=-1;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;

    CHECK_FUNC_ENSURE_EXIST(fs, statfs);

    if ( (ret=fs->fuse_operations->statfs(path, buf)) < 0){
	SET_ERRNO(-ret);
	return -1;
    }
    
    return ret;
}


static int fuse_operations_mount_stat(struct MountsPublicInterface* this_, const char* path, struct stat *buf){
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    GET_STAT_ENSURE_EXIST(fs, path, buf);
    return 0;
}


static int fuse_operations_mount_mkdir(struct MountsPublicInterface* this_, const char* path, uint32_t mode){
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    int ret=-1;

    CHECK_FUNC_ENSURE_EXIST(fs, mkdir);

    if ( (ret = fs->fuse_operations->mkdir(path, mode)) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}


static int fuse_operations_mount_rmdir(struct MountsPublicInterface* this_, const char* path){
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    int ret;

    CHECK_FUNC_ENSURE_EXIST(fs, rmdir);

    if ( (ret = fs->fuse_operations->rmdir(path)) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}

static ssize_t fuse_operations_mount_read(struct MountsPublicInterface* this_, int fd, void *buf, size_t nbytes){
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct OpenFileDescription* ofd = fs->handle_allocator->ofd(fd);
    if (ofd!=NULL)
	return this_->pread(this_, fd, buf, nbytes, ofd->offset);
    else{
	SET_ERRNO(ENOENT);
	return -1;
    }
}

static ssize_t fuse_operations_mount_write(struct MountsPublicInterface* this_, int fd, const void *buf, size_t nbytes){
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct OpenFileDescription* ofd = fs->handle_allocator->ofd(fd);
    if (ofd!=NULL)
	return this_->pwrite(this_, fd, buf, nbytes, ofd->offset);
    else{
	SET_ERRNO(ENOENT);
	return -1;
    }
}

static ssize_t 
fuse_operations_mount_pread(struct MountsPublicInterface* this_, 
	       int fd, void *buf, size_t nbytes, off_t offset){
    int ret;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct HandleItem *entry = fs->handle_allocator->entry(fd);
    const struct OpenFileDescription* ofd = fs->handle_allocator->ofd(fd);
    if (ofd==NULL){
	SET_ERRNO(ENOENT);
	return -1;
    }
    struct FuseFileOptionalData *fdata = (struct FuseFileOptionalData *)ofd->optional_data;

    CHECK_FUNC_ENSURE_EXIST(fs, read);

    if ( (ret=fs->fuse_operations->read(fdata->path, buf, nbytes, offset, fdata->finfo)) >= 0 ){
	/*update resulted offset*/
	int ret2 = fs->open_files_pool->set_offset(entry->open_file_description_id, offset+ret );
	assert(ret2==0);
    }
    else if ( ret < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }
    
    return ret;
}

static ssize_t 
fuse_operations_mount_pwrite(struct MountsPublicInterface* this_, 
		int fd, const void *buf, size_t nbytes, off_t offset){
    int ret;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct HandleItem* entry = fs->handle_allocator->entry(fd);
    const struct OpenFileDescription* ofd = fs->open_files_pool->entry(entry->open_file_description_id);
    if (ofd==NULL){
	SET_ERRNO(ENOENT);
	return -1;
    }
    struct FuseFileOptionalData *fdata = (struct FuseFileOptionalData *)ofd->optional_data;

    CHECK_FUNC_ENSURE_EXIST(fs, write);

    if ( (ret=fs->fuse_operations->write(fdata->path, buf, nbytes, offset, fdata->finfo)) >= 0 ){
	/*update resulted offset*/
	int ret2 = fs->open_files_pool->set_offset(entry->open_file_description_id, offset+ret );
	assert(ret2==0);
    }
    else if ( ret < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }
    
    return ret;
}

static int fuse_operations_mount_fchown(struct MountsPublicInterface* this_, int fd, uid_t owner, gid_t group){
    int ret;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct OpenFileDescription* ofd = fs->handle_allocator->ofd(fd);
    if (ofd==NULL){
	SET_ERRNO(ENOENT);
	return -1;
    }
    struct FuseFileOptionalData *fdata = (struct FuseFileOptionalData *)ofd->optional_data;

    CHECK_FUNC_ENSURE_EXIST(fs, chown);

    if ( (ret=fs->fuse_operations->chown( fdata->path, owner, group)) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}

static int fuse_operations_mount_fchmod(struct MountsPublicInterface* this_, int fd, uint32_t mode){
    int ret;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct OpenFileDescription* ofd = fs->handle_allocator->ofd(fd);
    if (ofd==NULL){
	SET_ERRNO(ENOENT);
	return -1;
    }
    struct FuseFileOptionalData *fdata = (struct FuseFileOptionalData *)ofd->optional_data;

    CHECK_FUNC_ENSURE_EXIST(fs, chmod);

    if ( (ret=fs->fuse_operations->chmod( fdata->path, mode)) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}


static int fuse_operations_mount_fstat(struct MountsPublicInterface* this_, int fd, struct stat *buf){
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct OpenFileDescription* ofd = fs->handle_allocator->ofd(fd);
    if (ofd==NULL){
	struct FuseFileOptionalData *fdata = (struct FuseFileOptionalData *)ofd->optional_data;
	GET_STAT_ENSURE_EXIST(fs, fdata->path, buf);
    }
    else{
	SET_ERRNO(ENOENT);
	return -1;
    }
    return 0;
}

static int fuse_readdir_filler(void *buf, const char *name, const struct stat *stbuf, off_t off){
    /*In our FUSE implementation formats of data for readdir, getdents
      are the same.*/
    struct DirentEnginePublicInterface* dirent_engine = get_dirent_engine();
    size_t required_size;
    size_t check_size = required_size = dirent_engine->adjusted_dirent_size( strlen(name) );
    /*check if free buffer buffer space is enough to place dirent */
    while(buf++ == '\0' && --check_size >0 );
    if (check_size==0){
	/*put just the same buffer size that is enough to fit dirent data*/
	ssize_t ret = dirent_engine->add_dirent_into_buf( buf,
							  required_size,
							  stbuf->st_ino, 
							  off, 
							  stbuf->st_mode, 
							  name );
	assert(ret==required_size);
	return 0;
    }
    else return 1;
}


static int fuse_operations_mount_getdents(struct MountsPublicInterface* this_, int fd, void *buf, unsigned int count){
    int ret;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct HandleItem* entry = fs->handle_allocator->entry(fd);
    const struct OpenFileDescription* ofd = fs->open_files_pool->entry(entry->open_file_description_id);
    if ( ofd==NULL ){
	SET_ERRNO(ENOENT);
	return -1;
    }
    struct FuseFileOptionalData *fdata = (struct FuseFileOptionalData *)ofd->optional_data;

    /*prepare buffer to be able determine size in filler function*/
    memset(buf, '\0', count-1);
    ((char*)buf)[count-1] = '@';

    ssize_t readed=0;

    /*In FUSE offset is not a byte offset, but offset is the count of
      items in getdents buffer. This count will be saved in separate
      field ofd->offset*/
    if ( (readed=fs->fuse_operations
	  ->readdir(fdata->path, buf, fuse_readdir_filler, ofd->offset, fdata->finfo)) < 0 ){
	SET_ERRNO(-readed);
    }

    if ( readed > 0 ){
	ret = fs->open_files_pool->set_offset( entry->open_file_description_id, 
					       ofd->offset + readed );
	assert( ret == 0 );

	/*determine filled buffer size by seeking null, this aproach
	  is slow in comparison with temp variable which won't use*/
	ret=0;
	while(buf++ != '\0' && ret++);
    }
    else
	ret=-1;

    return ret;
}

static int fuse_operations_mount_fsync(struct MountsPublicInterface* this_, int fd){
    errno=ENOSYS;
    return -1;
}

static int fuse_operations_mount_close(struct MountsPublicInterface* this_, int fd){
    int ret;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct HandleItem* entry = fs->handle_allocator->entry(fd);
    const struct OpenFileDescription* ofd = fs->open_files_pool->entry(entry->open_file_description_id);
    if (ofd==NULL){
	SET_ERRNO(ENOENT);
	return -1;
    }
    struct FuseFileOptionalData *fdata = (struct FuseFileOptionalData *)ofd->optional_data;

    CHECK_FUNC_ENSURE_EXIST(fs, flush);

    if ( (ret=fs->fuse_operations->flush( fdata->path, fdata->finfo)) <0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    if ( (ret=fs->fuse_operations->release( fdata->path, fdata->finfo)) <0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    ret=fs->open_files_pool->release_ofd(entry->open_file_description_id);    
    assert( ret == 0 );
    ret = fs->handle_allocator->free_handle(fd);
    free(fdata); /*fuse related object*/

    return ret;
}

static off_t fuse_operations_mount_lseek(struct MountsPublicInterface* this_, int fd, off_t offset, int whence){
    int ret;
    struct stat st;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct HandleItem* entry = fs->handle_allocator->entry(fd);
    const struct OpenFileDescription* ofd = fs->open_files_pool->entry(entry->open_file_description_id);
    if (ofd==NULL){
	SET_ERRNO(ENOENT);
	return -1;
    }
    struct FuseFileOptionalData *fdata = (struct FuseFileOptionalData *)ofd->optional_data;

    GET_STAT_ENSURE_EXIST(fs, fdata->path, &st);

    if ( !S_ISDIR(st.st_mode) ){
	SET_ERRNO(EBADF);
	return -1;
    }

    off_t next = ofd->offset;
    ssize_t len;
    switch (whence) {
    case SEEK_SET:
	next = offset;
	break;
    case SEEK_CUR:
	next += offset;
	break;
    case SEEK_END:
	len = st.st_size;
	if (len == -1) {
	    return -1;
	}
	next = (size_t)len + offset;
	break;
    default:
	SET_ERRNO(EINVAL);
	return -1;
    }
    // Must not seek beyond the front of the file.
    if (next < 0) {
	SET_ERRNO(EINVAL);
	return -1;
    }
    // Go to the new offset.
    ret = fs->open_files_pool->set_offset(entry->open_file_description_id, next);
    assert( ret == 0 );
    return next;
}

static int fuse_operations_mount_open(struct MountsPublicInterface* this_, const char* path, int oflag, uint32_t mode){
    int ret=-1;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    struct stat st;
    struct stat st_parent;
    int file_exist;

    CHECK_FUNC_ENSURE_EXIST(fs, open);

    file_exist=fs->fuse_operations->getattr(path, &st);
    /*if trying top open existing directoy*/
    if ( !file_exist || !S_ISDIR(st.st_mode) ){
	/*not suitable for opening dirs*/
	SET_ERRNO(EBADF);
	return -1;
    }

    /*get & check parent directory*/
    int cursor_path;
    int parent_dir_len;
    INIT_TEMP_CURSOR(&cursor_path);
    const char *parent_dir = path_subpath_backward(&cursor_path, path, &parent_dir_len);
    if ( parent_dir != NULL && fs->fuse_operations->getattr(parent_dir, &st_parent) != 0 ){
	/*it seems that trYing to open a root directory, folder
	  opening error is already handled*/
	ZRT_LOG(L_ERROR, "%s: parent_dir can't be obtained", __FILE__);
	assert(0);
    }

    /*file exist, do checks of O_CREAT, O_EXCL flags*/
    if ( !file_exist && CHECK_FLAG(oflag, O_CREAT) && CHECK_FLAG(oflag, O_EXCL) ){
	SET_ERRNO(EEXIST);
	return -1;
    }

    /*truncate existing writable file*/
    if ( !file_exist && CHECK_FLAG(oflag, O_TRUNC) && 
	 (CHECK_FLAG(oflag, O_WRONLY) || CHECK_FLAG(oflag, O_RDWR)) ){
	CHECK_FUNC_ENSURE_EXIST(fs, truncate);
	if ( (ret=fs->fuse_operations->truncate(path, 0)) <0 ){
	    /*truncate error*/
	    SET_ERRNO(-ret);
	    return -1;
	}
    }

    /*create fuse object, it must be attached into ofd entry or be
      destroyed if zrt would not be able to return file handle*/
    struct fuse_file_info *finfo = calloc(1, sizeof(struct fuse_file_info));
    /*reset flags that should not be passed to fuse's open*/
    finfo->flags = oflag & ~(O_CREAT|O_TRUNC|O_EXCL);

    /*is file exist?*/
    if (!file_exist){
	if ( (ret=fs->fuse_operations->open(path, finfo)) <0 ){
	    free(finfo);
	    SET_ERRNO(-ret);
	    return -1;
	}
    }
    else{ 
	/*file does not exist, create & open it*/
	if ( CHECK_FLAG(oflag, O_CREAT) ){
	    CHECK_FUNC_ENSURE_EXIST(fs, create);
	    if ( (ret=fs->fuse_operations->create(path, mode, finfo)) <0 ){
		free(finfo);
		SET_ERRNO(-ret);
		return -1;
	    }
	}
    }

    /*allocate global - zrt file handle*/
    int open_file_description_id = get_open_files_pool()->getnew_ofd(oflag);
    int global_fd = get_handle_allocator()->allocate_handle(this_, 
							    st.st_ino, st_parent.st_ino,
							    open_file_description_id);
    if ( global_fd < 0 ){
	/*it's hipotetical but possible case if amount of open files 
	  are exceeded an maximum value.*/
	get_open_files_pool()->release_ofd(open_file_description_id);
	/*also free fuse object*/
	free(finfo);
	SET_ERRNO(ENFILE);
	return -1;
    }

    /*Now file has a global file descriptor, will use ofd to save fuse object there*/
    const struct OpenFileDescription* ofd = fs->handle_allocator->ofd(global_fd);
    struct FuseFileOptionalData *fdata = (struct FuseFileOptionalData *)ofd->optional_data;
    fdata->path = strdup(path);
    fdata->finfo = finfo;

    /*success*/

    return ret;
}


static int fuse_operations_mount_fcntl(struct MountsPublicInterface* this_, int fd, int cmd, ...){
    ZRT_LOG(L_INFO, "cmd=%s", STR_FCNTL_CMD(cmd));
    SET_ERRNO(ENOSYS);
    return -1;
}

static int fuse_operations_mount_remove(struct MountsPublicInterface* this_, const char* path){
    int ret=-1;
    struct stat st;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;

    GET_STAT_ENSURE_EXIST(fs, path, &st);

    if ( S_ISDIR(st.st_mode) ){
	CHECK_FUNC_ENSURE_EXIST(fs, rmdir);
	if ( (ret=fs->fuse_operations->rmdir( path )) < 0 ){
	    SET_ERRNO(-ret);
	    return -1;
	}
    }
    else{
	CHECK_FUNC_ENSURE_EXIST(fs, unlink);
	if ( (ret=fs->fuse_operations->unlink( path )) <= 0 ){
	    SET_ERRNO(-ret);
	    return -1;
	}
    }
    return ret;
}

static int fuse_operations_mount_unlink(struct MountsPublicInterface* this_, const char* path){
    int ret=-1;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;

    CHECK_FUNC_ENSURE_EXIST(fs, unlink);

    if ( (ret=fs->fuse_operations->unlink( path )) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}

static int fuse_operations_mount_access(struct MountsPublicInterface* this_, const char* path, int amode){
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    int ret = -1;

    /*If access function not defined just pass OK result*/
    if ( fs->fuse_operations->access ){
	if ( (ret=fs->fuse_operations->access(path, amode)) < 0){
	    SET_ERRNO(-ret);
	    return -1;
	}
    }
    else return 0;

    return ret;
}

static int fuse_operations_mount_ftruncate_size(struct MountsPublicInterface* this_, int fd, off_t length){
    int ret;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    const struct OpenFileDescription* ofd = fs->handle_allocator->ofd(fd);
    if (ofd==NULL){
	SET_ERRNO(ENOENT);
	return -1;
    }
    struct FuseFileOptionalData *fdata = (struct FuseFileOptionalData *)ofd->optional_data;
    struct stat st;

    CHECK_FUNC_ENSURE_EXIST(fs, ftruncate);
    GET_STAT_ENSURE_EXIST(fs, fdata->path, &st);

    if ( S_ISDIR(st.st_mode) ){
	SET_ERRNO(EISDIR);
	return -1;
    }

    int flags = ofd->flags & O_ACCMODE;
    /*check if file was not opened for writing*/
    if ( flags!=O_WRONLY && flags!=O_RDWR ){
	ZRT_LOG(L_ERROR, "file open flags=%s not allow truncate", 
		STR_FILE_OPEN_FLAGS(flags));
	SET_ERRNO( EINVAL );
	return -1;
    }

    if ( (ret=fs->fuse_operations->ftruncate(fdata->path, length, fdata->finfo )) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}

int fuse_operations_mount_truncate_size(struct MountsPublicInterface* this_, const char* path, off_t length){
    int ret;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    struct stat st;

    CHECK_FUNC_ENSURE_EXIST(fs, truncate);
    GET_STAT_ENSURE_EXIST(fs, path, &st);

    if ( S_ISDIR(st.st_mode) ){
	SET_ERRNO(EISDIR);
	return -1;
    }

    if ( (ret=fs->fuse_operations->truncate( path, length )) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}

static int fuse_operations_mount_isatty(struct MountsPublicInterface* this_, int fd){
    return -1;
}

static int fuse_operations_mount_dup(struct MountsPublicInterface* this_, int oldfd){
    return -1;
}

static int fuse_operations_mount_dup2(struct MountsPublicInterface* this_, int oldfd, int newfd){
    return -1;
}

static int fuse_operations_mount_link(struct MountsPublicInterface* this_, const char* oldpath, const char* newpath){
    int ret;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    struct stat st_old;
    struct stat st_new_parent;

    CHECK_FUNC_ENSURE_EXIST(fs, link);
    GET_STAT_ENSURE_EXIST(fs, oldpath, &st_old);
    GET_PARENT_STAT_ENSURE_EXIST(fs, newpath, &st_new_parent);

    if ( (ret=fs->fuse_operations->link( oldpath, newpath )) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}


static int fuse_operations_mount_rename(struct MountsPublicInterface* this_, const char* oldpath, const char* newpath){
    int ret;
    struct FuseOperationsMount* fs = (struct FuseOperationsMount*)this_;
    struct stat st_old;

    CHECK_FUNC_ENSURE_EXIST(fs, rename);
    GET_STAT_ENSURE_EXIST(fs, oldpath, &st_old);

    if ( (ret=fs->fuse_operations->rename( oldpath, newpath )) < 0 ){
	SET_ERRNO(-ret);
	return -1;
    }

    return ret;
}


static struct MountsPublicInterface KFuseOperationsMount = {
    fuse_operations_mount_readlink,
    fuse_operations_mount_symlink,
    fuse_operations_mount_chown,
    fuse_operations_mount_chmod,
    fuse_operations_mount_statvfs,
    fuse_operations_mount_stat,
    fuse_operations_mount_mkdir,
    fuse_operations_mount_rmdir,
    fuse_operations_mount_read,
    fuse_operations_mount_write,
    fuse_operations_mount_pread,
    fuse_operations_mount_pwrite,
    fuse_operations_mount_fchown,
    fuse_operations_mount_fchmod,
    fuse_operations_mount_fstat,
    fuse_operations_mount_getdents,
    fuse_operations_mount_fsync,
    fuse_operations_mount_close,
    fuse_operations_mount_lseek,
    fuse_operations_mount_open,
    fuse_operations_mount_fcntl,
    fuse_operations_mount_remove,
    fuse_operations_mount_unlink,
    fuse_operations_mount_rename,
    fuse_operations_mount_access,
    fuse_operations_mount_ftruncate_size,
    fuse_operations_mount_truncate_size,
    fuse_operations_mount_isatty,
    fuse_operations_mount_dup,
    fuse_operations_mount_dup2,
    fuse_operations_mount_link,
    EUserFsId,
    NULL
};

struct MountsPublicInterface* 
CONSTRUCT_L(FUSE_OPERATIONS_MOUNT)( struct HandleAllocator* handle_allocator,
				   struct OpenFilesPool* open_files_pool,
				   struct fuse_operations* fuse_operations){
    /*use malloc and not new, because it's external c object*/
    struct FuseOperationsMount* this_ = (struct FuseOperationsMount*)malloc( sizeof(struct FuseOperationsMount) );

    /*set functions*/
    this_->public_ = KFuseOperationsMount;
    /*set data members*/
    this_->handle_allocator = handle_allocator; /*use existing handle allocator*/
    this_->open_files_pool = open_files_pool; /*use existing open files pool*/
    this_->fuse_operations = fuse_operations;
    return (struct MountsPublicInterface*)this_;
}

