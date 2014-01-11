/*
 * mem_mount.c
 *
 *  Created on: 20.09.2012
 *      Author: yaroslav
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>

extern "C" {
#include "zrtlog.h"
#include "zrt_helper_macros.h"
}
#include "nacl-mounts/memory/MemMount.h"
#include "nacl-mounts/util/Path.h"
#include "mem_mount_wraper.h"
#include "mount_specific_implem.h"
extern "C" {
#include "handle_allocator.h"
#include "fstab_observer.h" /*lazy mount*/
#include "fcntl_implem.h"
#include "channels_mount.h"
#include "enum_strings.h"
}

#define NODE_OBJECT_BYINODE(inode) s_mem_mount_cpp->ToMemNode(inode)
#define NODE_OBJECT_BYPATH(path) s_mem_mount_cpp->GetMemNode(path)

/*retcode -1 at fail, 0 if OK*/
#define GET_INODE_BY_HANDLE(handle, inode_p, retcode_p){		\
	*retcode_p = s_handle_allocator->get_inode( handle, inode_p );	\
    }

#define GET_INODE_BY_HANDLE_OR_RAISE_ERROR(handle, inode_p){		\
	int ret;							\
	GET_INODE_BY_HANDLE(handle, inode_p, &ret);			\
	if ( ret != 0 ){						\
	    SET_ERRNO(EBADF);						\
	    return -1;							\
	}								\
    }

#define GET_STAT_BYPATH_OR_RAISE_ERROR(path, stat_p ){		\
	int ret = s_mem_mount_cpp->GetNode( path, stat_p);	\
	if ( ret != 0 ) return ret;				\
    }


static MemMount* s_mem_mount_cpp = NULL;
static struct HandleAllocator* s_handle_allocator = NULL;
static struct MountsPublicInterface* s_this=NULL;


static const char* name_from_path( std::string path ){
    /*retrieve directory name, and compare name length with max available*/
    size_t pos=path.rfind ( '/' );
    int namelen = 0;
    if ( pos != std::string::npos ){
	namelen = path.length() -(pos+1);
	return path.c_str()+path.length()-namelen;
    }
    return NULL;
}

static int is_dir( ino_t inode ){
    struct stat st;
    int ret = s_mem_mount_cpp->Stat( inode, &st );
    assert( ret == 0 );
    if ( S_ISDIR(st.st_mode) )
	return 1;
    else
	return 0;
}

static ssize_t get_file_len(ino_t node) {
    struct stat st;
    if (0 != s_mem_mount_cpp->Stat(node, &st)) {
	return -1;
    }
    return (ssize_t) st.st_size;
}

/*mount specific implementation*/

/* MemMount specific implementation functions intended to initialize
 * mount_specific_implem struct pointers;
 * */

/*return 0 if handle not valid, or 1 if handle is correct*/
static int check_handle(struct MountSpecificImplemPublicInterface* this_, int handle){
    ino_t inode;
    int ret;
    GET_INODE_BY_HANDLE(handle, &inode, &ret);
    if ( ret == 0 && !is_dir(inode) ) return 1;
    else return 0;
}

static const char* path_handle(struct MountSpecificImplemPublicInterface* this_, int handle){
    ino_t inode;
    int ret;
    GET_INODE_BY_HANDLE(handle, &inode, &ret);
    if ( ret == 0 ){
	/*path is OK*/
	/*get runtime information related to channel*/
    	MemNode* mnode = NODE_OBJECT_BYINODE(inode);
	if ( mnode ){
	    return mnode->name().c_str();
	}
	else
	    return NULL;
    }
    else{
	return NULL;
    }
}

static int file_status_flags(struct MountSpecificImplemPublicInterface* this_, int fd){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);
    MemNode* mnode = NODE_OBJECT_BYINODE(inode);
    if ( mnode ){
	return mnode->flags();
    }
    assert(0);
}

static int set_file_status_flags(struct MountSpecificImplemPublicInterface* this_, int fd, int flags){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);
    MemNode* mnode = NODE_OBJECT_BYINODE(inode);
    if ( mnode ){
	mnode->set_flags(flags);
	return 0;
    }
    assert(0);
}


/*return pointer at success, NULL if fd didn't found or flock structure has not been set*/
static const struct flock* flock_data(struct MountSpecificImplemPublicInterface* this_, int fd ){
    const struct flock* data = NULL;
    ino_t inode;
    int ret;
    GET_INODE_BY_HANDLE(fd, &inode, &ret);
    if ( ret == 0 ){
	/*handle is OK*/
	/*get runtime information related to channel*/
    	MemNode* mnode = NODE_OBJECT_BYINODE(inode);
	assert(mnode);
	data = mnode->flock();
    }
    return data;
}

/*return 0 if success, -1 if fd didn't found*/
static int set_flock_data(struct MountSpecificImplemPublicInterface* this_, int fd, const struct flock* flock_data ){
    ino_t inode;
    int ret;
    GET_INODE_BY_HANDLE(fd, &inode, &ret);
    if ( ret !=0 ) return -1;

    /*handle is OK*/
    /*get runtime information related to channel*/
    MemNode* mnode = NODE_OBJECT_BYINODE(inode);
    assert(mnode);
    mnode->set_flock(flock_data);
    return 0; /*OK*/
}

static struct MountSpecificImplemPublicInterface s_mount_specific_implem = {
    check_handle,
    path_handle,
    file_status_flags,
    set_file_status_flags,
    flock_data,
    set_flock_data
};


/*helpers*/

/*@return 0 if success, -1 if we don't need to mount*/
static int lazy_mount(const char* path){
    struct stat st;
    int ret = s_mem_mount_cpp->GetNode( path, &st);
    (void)ret;
    /*if it's time to do mount, then do all waiting mounts*/
    FstabObserver* observer = get_fstab_observer();
    struct FstabRecordContainer* record;
    while( NULL != (record = observer->locate_postpone_mount( observer, path, 
							      EFstabMountWaiting)) ){
	observer->mount_import(observer, record);
	return 0;
    }
    return -1;
}


/*wraper implementation*/

static int mem_chown(struct MountsPublicInterface* this_, const char* path, uid_t owner, gid_t group){
    lazy_mount(path);
    struct stat st;
    GET_STAT_BYPATH_OR_RAISE_ERROR(path, &st);
    return s_mem_mount_cpp->Chown( st.st_ino, owner, group);
}

static int mem_chmod(struct MountsPublicInterface* this_, const char* path, uint32_t mode){
    lazy_mount(path);
    struct stat st;
    GET_STAT_BYPATH_OR_RAISE_ERROR(path, &st);
    return s_mem_mount_cpp->Chmod( st.st_ino, mode);
}

static int mem_stat(struct MountsPublicInterface* this_, const char* path, struct stat *buf){
    lazy_mount(path);
    struct stat st;
    GET_STAT_BYPATH_OR_RAISE_ERROR(path, &st);
    int ret = s_mem_mount_cpp->Stat( st.st_ino, buf);

    /*fixes for hardlinks pseudo support, different hardlinks must have same inode,
     *but internally all nodes have separeted inodes*/
    if ( ret == 0 ){
	MemNode* node = NODE_OBJECT_BYINODE(st.st_ino);
	assert(node!=NULL);
	/*patch inode if it has hardlinks*/
	if ( node->hardinode() > 0 )
	    buf->st_ino = (ino_t)node->hardinode();
    }

    return ret;
}

static int mem_mkdir(struct MountsPublicInterface* this_, const char* path, uint32_t mode){
    lazy_mount(path);
    int ret = s_mem_mount_cpp->GetNode( path, NULL);
    if ( ret == 0 || (ret == -1&&errno==ENOENT) )
	return s_mem_mount_cpp->Mkdir( path, mode, NULL);
    else{
	return ret;
    }
}


static int mem_rmdir(struct MountsPublicInterface* this_, const char* path){
    lazy_mount(path);
    struct stat st;
    GET_STAT_BYPATH_OR_RAISE_ERROR(path, &st);

    const char* name = name_from_path(path);
    ZRT_LOG( L_EXTRA, "name=%s", name );
    if ( name && !strcmp(name, ".\0")  ){
	/*should be specified real path for rmdir*/
	SET_ERRNO(EINVAL);
	return -1;
    }
    return s_mem_mount_cpp->Rmdir( st.st_ino );
}

static int mem_umount(struct MountsPublicInterface* this_, const char* path){
    SET_ERRNO(ENOSYS);
    return -1;
}

static int mem_mount(struct MountsPublicInterface* this_, const char* path, void *mount){
    SET_ERRNO(ENOSYS);
    return -1;
}

static ssize_t mem_read(struct MountsPublicInterface* this_, int fd, void *buf, size_t nbyte){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);

    off_t offset;
    int ret = s_handle_allocator->get_offset( fd, &offset );
    assert( ret == 0 );
    ssize_t readed = s_mem_mount_cpp->Read( inode, offset, buf, nbyte );
    if ( readed >= 0 ){
	offset += readed;
	/*update offset*/
	ret = s_handle_allocator->set_offset( fd, offset );
	assert( ret == 0 );
    }
    /*return readed bytes or error*/
    return readed;
}

static ssize_t mem_write(struct MountsPublicInterface* this_, int fd, const void *buf, size_t nbyte){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);

    off_t offset;
    int ret = s_handle_allocator->get_offset( fd, &offset );
    assert( ret == 0 );
    ssize_t wrote = s_mem_mount_cpp->Write( inode, offset, buf, nbyte );
    if ( wrote != -1 ){
	offset += wrote;
	ret = s_handle_allocator->set_offset( fd, offset );
	assert( ret == 0 );
    }
    return wrote;
}

static int mem_fchown(struct MountsPublicInterface* this_, int fd, uid_t owner, gid_t group){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);
    return s_mem_mount_cpp->Chown( inode, owner, group);
}

static int mem_fchmod(struct MountsPublicInterface* this_, int fd, uint32_t mode){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);
    return s_mem_mount_cpp->Chmod( inode, mode);
}


static int mem_fstat(struct MountsPublicInterface* this_, int fd, struct stat *buf){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);
    return s_mem_mount_cpp->Stat( inode, buf);
}

static int mem_getdents(struct MountsPublicInterface* this_, int fd, void *buf, unsigned int count){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);

    off_t offset;
    int ret = s_handle_allocator->get_offset( fd, &offset );
    assert( ret == 0 );
    ssize_t readed = s_mem_mount_cpp->Getdents( inode, offset, (DIRENT*)buf, count);
    if ( readed != -1 ){
	offset += readed;
	ret = s_handle_allocator->set_offset( fd, offset );
	assert( ret == 0 );
    }
    return readed;
}

static int mem_fsync(struct MountsPublicInterface* this_, int fd){
    errno=ENOSYS;
    return -1;
}

static int mem_close(struct MountsPublicInterface* this_, int fd){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);
    MemNode* mnode = NODE_OBJECT_BYINODE(inode);
    assert(mnode);

    s_mem_mount_cpp->Unref(mnode->slot()); /*decrement use count*/
    if ( mnode->UnlinkisTrying() ){
	if( s_mem_mount_cpp->UnlinkInternal(mnode) == 0 ){
	    mnode->UnlinkOkResetFlag();
	}
    }
    
    int ret = s_handle_allocator->free_handle(fd);
    assert( ret == 0 );
    return 0;
}

static off_t mem_lseek(struct MountsPublicInterface* this_, int fd, off_t offset, int whence){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);

    if ( !is_dir(inode) ){
	off_t next;
	int ret = s_handle_allocator->get_offset(fd, &next );
	assert( ret == 0 );
	ssize_t len;

	switch (whence) {
	case SEEK_SET:
	    next = offset;
	    break;
	case SEEK_CUR:
	    next += offset;
	    // TODO(arbenson): handle EOVERFLOW if too big.
	    break;
	case SEEK_END:
	    // TODO(krasin, arbenson): FileHandle should store file len.
	    len = get_file_len( inode );
	    if (len == -1) {
		return -1;
	    }
	    next = static_cast<size_t>(len) + offset;
	    // TODO(arbenson): handle EOVERFLOW if too big.
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
	ret = s_handle_allocator->set_offset(fd, next );
	assert( ret == 0 );
	return next;
    }
    else{
	SET_ERRNO(EBADF);
	return -1;
    }
}

static int mem_open(struct MountsPublicInterface* this_, const char* path, int oflag, uint32_t mode){
    lazy_mount(path);
    int ret = s_mem_mount_cpp->Open(path, oflag, mode);

    /* get node from memory FS for specified type, if no errors occured 
     * then file allocated in memory FS and require file desc - fd*/
    struct stat st;
    if ( ret == 0 && s_mem_mount_cpp->GetNode( path, &st) == 0 ){
	/*ask for file descriptor in handle allocator*/
	int fd = s_handle_allocator->allocate_handle( s_this );
	if ( fd < 0 ){
	    /*it's hipotetical but possible case if amount of open files 
	      are exceeded an maximum value*/
	    SET_ERRNO(ENFILE);
	    return -1;
	}
	
	/* As inode and fd are depend each form other, we need update 
	 * inode in handle allocator and stay them linked*/
	ret = s_handle_allocator->set_inode( fd, st.st_ino );
	ZRT_LOG(L_EXTRA, "errcode ret=%d", ret );
	assert( ret == 0 );

	/*append feature support, is simple*/
	if ( oflag & O_APPEND ){
	    ZRT_LOG(L_SHORT, P_TEXT, "handle flag: O_APPEND");
	    mem_lseek(this_, fd, 0, SEEK_END);
	}

	/*file truncate support, only for writable files, reset size*/
	if ( oflag&O_TRUNC && (oflag&O_RDWR || oflag&O_WRONLY) ){
	    /*reset file size*/
	    MemNode* mnode = NODE_OBJECT_BYINODE(st.st_ino);
	    if (mnode){ 
		ZRT_LOG(L_SHORT, P_TEXT, "handle flag: O_TRUNC");
		/*update stat*/
		st.st_size = 0;
		mnode->set_len(st.st_size);
		ZRT_LOG(L_SHORT, "%s, %d", mnode->name().c_str(), mnode->len() );
	    }
	}

	/*success*/
	return fd;
    }
    else
	return -1;
}

static int mem_fcntl(struct MountsPublicInterface* this_, int fd, int cmd, ...){
    ino_t inode;
    ZRT_LOG(L_INFO, "fcntl cmd=%s", STR_FCNTL_CMD(cmd));
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);
    if ( is_dir(inode) ){
	SET_ERRNO(EBADF);
	return -1;
    }
    return 0;
}

static int mem_remove(struct MountsPublicInterface* this_, const char* path){
    return s_mem_mount_cpp->Unlink(path);
}

static int mem_unlink(struct MountsPublicInterface* this_, const char* path){
    return s_mem_mount_cpp->Unlink(path);
}

static int mem_access(struct MountsPublicInterface* this_, const char* path, int amode){
    return -1;
}

static int mem_ftruncate_size(struct MountsPublicInterface* this_, int fd, off_t length){
    ino_t inode;
    MemNode* node; 
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR(fd, &inode);
    if ( !is_dir(inode) ){
	if ((node=NODE_OBJECT_BYINODE( inode )) != NULL){
	    /*check if file was opened for writing*/
	    int flags= node->flags();
	    if ( !CHECK_FLAG(flags, O_WRONLY) && !CHECK_FLAG(flags, O_RDWR) ){
		ZRT_LOG(L_ERROR, "file not allowed for write, open flags are %s",
			STR_FILE_OPEN_FLAGS(flags));
		SET_ERRNO( EINVAL );
		return -1;
	    }
	    /*set file length on related node and update new length in stat*/
	    node->set_len(length);

	    /*in according to docs if reducing file offset should not
	     be changed, but on ubuntu linux host offset is not staying
	     beyond of file bounds and assignes to max availibale
	     pos. Juyt do it the same as on host */
#define DO_NOT_ALLOW_OFFSET_BEYOND_FILE_BOUNDS_IF_TRUNCATE_REDUCES_FILE_SIZE

#ifdef DO_NOT_ALLOW_OFFSET_BEYOND_FILE_BOUNDS_IF_TRUNCATE_REDUCES_FILE_SIZE
	    off_t offset;
	    int ret = s_handle_allocator->get_offset(fd, &offset );
	    assert(ret==0);
	    if ( length < offset ){
		offset = length+1;
		ret = s_handle_allocator->set_offset(fd, offset );
		assert( ret == 0 );
	    }
#endif //DO_NOT_ALLOW_OFFSET_BEYOND_FILE_BOUNDS_IF_TRUNCATE_REDUCES_FILE_SIZE

	    ZRT_LOG(L_SHORT, "file truncated on %d len, updated st.size=%d",
		    (int)length, get_file_len( inode ));
	    /*file size truncated */
	}
	return 0;
    }
    else{
	SET_ERRNO(EBADF);
	return -1;
    }
    return -1;
}

int mem_truncate_size(struct MountsPublicInterface* this_, const char* path, off_t length){
    assert(0); 
    /*truncate implementation replaced by ftruncate call*/
    return -1;
}

static int mem_isatty(struct MountsPublicInterface* this_, int fd){
    return -1;
}

static int mem_dup(struct MountsPublicInterface* this_, int oldfd){
    return -1;
}

static int mem_dup2(struct MountsPublicInterface* this_, int oldfd, int newfd){
    return -1;
}

static int mem_link(struct MountsPublicInterface* this_, const char* oldpath, const char* newpath){
    lazy_mount(oldpath);
    lazy_mount(newpath);
    /*create new hardlink*/
    int ret = s_mem_mount_cpp->Link(oldpath, newpath);
    if ( ret == -1 ){
	/*errno already setted by MemMount*/
	return ret;
    }

    /*ask for file descriptor in handle allocator*/
    int fd = s_handle_allocator->allocate_handle( s_this );
    if ( fd < 0 ){
	/*it's hipotetical but possible case if amount of open files 
	  are exceeded an maximum value*/
	SET_ERRNO(ENFILE);
	return -1;
    }

    MemNode* mnode = NODE_OBJECT_BYPATH(newpath);
    assert(mnode);

    /* As inode and fd are depend each form other, we need update 
     * inode in handle allocator and stay them linked*/
    ret = s_handle_allocator->set_inode( fd, mnode->slot() );
    ZRT_LOG(L_EXTRA, "errcode ret=%d", ret );
    assert( ret == 0 );

    return 0;
}

struct MountSpecificImplemPublicInterface* mem_implem(struct MountsPublicInterface* this_){
    return &s_mount_specific_implem;
}

static struct MountsPublicInterface s_mem_mount_wraper = {
    mem_chown,
    mem_chmod,
    mem_stat,
    mem_mkdir,
    mem_rmdir,
    mem_umount,
    mem_mount,
    mem_read,
    mem_write,
    mem_fchown,
    mem_fchmod,
    mem_fstat,
    mem_getdents,
    mem_fsync,
    mem_close,
    mem_lseek,
    mem_open,
    mem_fcntl,
    mem_remove,
    mem_unlink,
    mem_access,
    mem_ftruncate_size,
    mem_truncate_size,
    mem_isatty,
    mem_dup,
    mem_dup2,
    mem_link,
    EMemMountId,
    mem_implem  /*mount_specific_implem interface*/
};

struct MountsPublicInterface* alloc_mem_mount( struct HandleAllocator* handle_allocator ){
    s_handle_allocator = handle_allocator;
    s_mem_mount_cpp = new MemMount;
    s_this = &s_mem_mount_wraper;
    return &s_mem_mount_wraper;
}

