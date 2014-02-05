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
#include "mounts_interface.h"
#include "mount_specific_interface.h"
extern "C" {
#include "handle_allocator.h"
#include "fstab_observer.h" /*lazy mount*/
#include "fcntl_implem.h"
#include "channels_mount.h"
#include "enum_strings.h"
}

#define NODE_OBJECT_BYINODE(memount_p, inode) memount_p->ToMemNode(inode)
#define NODE_OBJECT_BYPATH(memount_p, path) memount_p->GetMemNode(path)

/*retcode -1 at fail, 0 if OK*/
#define GET_INODE_BY_HANDLE(halloc_p, handle, inode_p, retcode_p){	\
	*retcode_p = halloc_p->get_inode( handle, inode_p );		\
    }

#define GET_INODE_BY_HANDLE_OR_RAISE_ERROR(halloc_p, handle, inode_p){	\
	int ret;							\
	GET_INODE_BY_HANDLE(halloc_p, handle, inode_p, &ret);		\
	if ( ret != 0 ){						\
	    SET_ERRNO(EBADF);						\
	    return -1;							\
	}								\
    }

#define GET_STAT_BYPATH_OR_RAISE_ERROR(memount_p, path, stat_p ){	\
	int ret = memount_p->GetNode( path, stat_p);			\
	if ( ret != 0 ) return ret;					\
    }

#define HALLOCATOR_BY_MOUNT_SPECIF(mount_specific_interface_p)		\
    ((struct InMemoryMounts*)((struct MountSpecificImplem*)(mount_specific_interface_p))->mount)->handle_allocator

#define MOUNT_INTERFACE_BY_MOUNT_SPECIF(mount_specific_interface_p)	\
    ((struct MountsPublicInterface*)((struct MountSpecificImplem*)(mount_specific_interface_p))->mount)

#define MEMOUNT_BY_MOUNT_SPECIF(mount_specific_interface_p)		\
    MEMOUNT_BY_MOUNT( MOUNT_INTERFACE_BY_MOUNT_SPECIF(mount_specific_interface_p) )

#define HALLOCATOR_BY_MOUNT(mount_interface_p)				\
    ((struct InMemoryMounts*)(mount_interface_p))->handle_allocator

#define MEMOUNT_BY_MOUNT(mounts_interface_p) ((struct InMemoryMounts*)mounts_interface_p)->mem_mount_cpp

#define CHECK_FILE_OPEN_FLAGS_OR_RAISE_ERROR(flags, flag1, flag2)	\
    if ( (flags)!=flag1 && (flags)!=flag2 ){				\
	ZRT_LOG(L_ERROR, "file open flags must be %s or %s",		\
		STR_FILE_OPEN_FLAGS(flag1), STR_FILE_OPEN_FLAGS(flag2)); \
	SET_ERRNO( EINVAL );						\
	return -1;							\
    }


struct InMemoryMounts{
    struct MountsPublicInterface public_;
    struct HandleAllocator* handle_allocator;
    MemMount*               mem_mount_cpp;
    struct MountSpecificPublicInterface* mount_specific_interface;
};


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

static int is_dir( struct MountsPublicInterface* this_, ino_t inode ){
    struct stat st;
    int ret = ((struct InMemoryMounts*)this_)->mem_mount_cpp->Stat( inode, &st );
    assert( ret == 0 );
    if ( S_ISDIR(st.st_mode) )
	return 1;
    else
	return 0;
}

static ssize_t get_file_len( struct MountsPublicInterface* this_, ino_t node) {
    struct stat st;
    if (0 != ((struct InMemoryMounts*)this_)->mem_mount_cpp->Stat(node, &st)) {
	return -1;
    }
    return (ssize_t) st.st_size;
}


/***********************************************************************
   implementation of MountSpecificPublicInterface as part of
   filesystem.  Below resides channels specific functions.*/

struct MountSpecificImplem{
    struct MountSpecificPublicInterface public_;
    struct InMemoryMounts* mount;
};

/*return 0 if handle not valid, or 1 if handle is correct*/
static int check_handle(struct MountSpecificPublicInterface* this_, int handle){
    ino_t inode;
    int ret;
    GET_INODE_BY_HANDLE( HALLOCATOR_BY_MOUNT_SPECIF(this_), handle, &inode, &ret);
    if ( ret == 0 && !is_dir( MOUNT_INTERFACE_BY_MOUNT_SPECIF(this_), inode) ) return 1;
    else return 0;
}

static const char* path_handle(struct MountSpecificPublicInterface* this_, int handle){
    ino_t inode;
    int ret;
    GET_INODE_BY_HANDLE( HALLOCATOR_BY_MOUNT_SPECIF(this_), handle, &inode, &ret);
    if ( ret == 0 ){
	/*path is OK*/
	/*get runtime information related to channel*/
    	MemNode* mnode = NODE_OBJECT_BYINODE( MEMOUNT_BY_MOUNT_SPECIF(this_), inode);
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

static int file_status_flags(struct MountSpecificPublicInterface* this_, int fd){
    ino_t inode;
    int flags;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT_SPECIF(this_), fd, &inode);

    HALLOCATOR_BY_MOUNT_SPECIF(this_)->get_flags( fd, &flags );
    return flags;
}

static int set_file_status_flags(struct MountSpecificPublicInterface* this_, int fd, int flags){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT_SPECIF(this_), fd, &inode);

    HALLOCATOR_BY_MOUNT_SPECIF(this_)->set_flags( fd, flags );
    return flags;
}


/*return pointer at success, NULL if fd didn't found or flock structure has not been set*/
static const struct flock* flock_data(struct MountSpecificPublicInterface* this_, int fd ){
    const struct flock* data = NULL;
    ino_t inode;
    int ret;
    GET_INODE_BY_HANDLE(HALLOCATOR_BY_MOUNT_SPECIF(this_), fd, &inode, &ret);
    if ( ret == 0 ){
	/*handle is OK*/
	/*get runtime information related to channel*/
    	MemNode* mnode = NODE_OBJECT_BYINODE( MEMOUNT_BY_MOUNT_SPECIF(this_), inode);
	assert(mnode);
	data = mnode->flock();
    }
    return data;
}

/*return 0 if success, -1 if fd didn't found*/
static int set_flock_data(struct MountSpecificPublicInterface* this_, int fd, const struct flock* flock_data ){
    ino_t inode;
    int ret;
    GET_INODE_BY_HANDLE(HALLOCATOR_BY_MOUNT_SPECIF(this_), fd, &inode, &ret);
    if ( ret !=0 ) return -1;

    /*handle is OK*/
    /*get runtime information related to channel*/
    MemNode* mnode = NODE_OBJECT_BYINODE( MEMOUNT_BY_MOUNT_SPECIF(this_), inode);
    assert(mnode);
    mnode->set_flock(flock_data);
    return 0; /*OK*/
}

static struct MountSpecificPublicInterface KMountSpecificImplem = {
    check_handle,
    path_handle,
    file_status_flags,
    set_file_status_flags,
    flock_data,
    set_flock_data
};


static struct MountSpecificPublicInterface*
mount_specific_construct( struct MountSpecificPublicInterface* specific_implem_interface,
			  struct InMemoryMounts* mount ){
    struct MountSpecificImplem* this_ = (struct MountSpecificImplem*)malloc(sizeof(struct MountSpecificImplem));
    /*set functions*/
    this_->public_ = *specific_implem_interface;
    this_->mount = mount;
    return (struct MountSpecificPublicInterface*)this_;
}


/*helpers*/

/*@return 0 if success, -1 if we don't need to mount*/
static int lazy_mount( struct MountsPublicInterface* this_, const char* path){
    struct stat st;
    int ret = MEMOUNT_BY_MOUNT(this_)->GetNode( path, &st);
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
    lazy_mount(this_, path);
    struct stat st;
    GET_STAT_BYPATH_OR_RAISE_ERROR( MEMOUNT_BY_MOUNT(this_), path, &st);
    return MEMOUNT_BY_MOUNT(this_)->Chown( st.st_ino, owner, group);
}

static int mem_chmod(struct MountsPublicInterface* this_, const char* path, uint32_t mode){
    lazy_mount(this_, path);
    struct stat st;
    GET_STAT_BYPATH_OR_RAISE_ERROR( MEMOUNT_BY_MOUNT(this_), path, &st);
    return MEMOUNT_BY_MOUNT(this_)->Chmod( st.st_ino, mode);
}

static int mem_stat(struct MountsPublicInterface* this_, const char* path, struct stat *buf){
    lazy_mount(this_, path);
    struct stat st;
    GET_STAT_BYPATH_OR_RAISE_ERROR( MEMOUNT_BY_MOUNT(this_), path, &st);
    int ret = MEMOUNT_BY_MOUNT(this_)->Stat( st.st_ino, buf);

    /*fixes for hardlinks pseudo support, different hardlinks must have same inode,
     *but internally all nodes have separeted inodes*/
    if ( ret == 0 ){
	MemNode* node = NODE_OBJECT_BYINODE( MEMOUNT_BY_MOUNT(this_), st.st_ino);
	assert(node!=NULL);
	/*patch inode if it has hardlinks*/
	if ( node->hardinode() > 0 )
	    buf->st_ino = (ino_t)node->hardinode();
    }

    return ret;
}

static int mem_mkdir(struct MountsPublicInterface* this_, const char* path, uint32_t mode){
    lazy_mount(this_, path);
    int ret = MEMOUNT_BY_MOUNT(this_)->GetNode( path, NULL);
    if ( ret == 0 || (ret == -1&&errno==ENOENT) )
	return MEMOUNT_BY_MOUNT(this_)->Mkdir( path, mode, NULL);
    else{
	return ret;
    }
}


static int mem_rmdir(struct MountsPublicInterface* this_, const char* path){
    lazy_mount(this_, path);
    struct stat st;
    GET_STAT_BYPATH_OR_RAISE_ERROR( MEMOUNT_BY_MOUNT(this_), path, &st);

    const char* name = name_from_path(path);
    ZRT_LOG( L_EXTRA, "name=%s", name );
    if ( name && !strcmp(name, ".\0")  ){
	/*should be specified real path for rmdir*/
	SET_ERRNO(EINVAL);
	return -1;
    }
    return MEMOUNT_BY_MOUNT(this_)->Rmdir( st.st_ino );
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
    off_t offset;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);
    HALLOCATOR_BY_MOUNT(this_)->get_offset( fd, &offset );
    return this_->pread(this_, fd, buf, nbyte, offset);
}

static ssize_t mem_write(struct MountsPublicInterface* this_, int fd, const void *buf, size_t nbyte){
    ino_t inode;
    off_t offset;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);
    HALLOCATOR_BY_MOUNT(this_)->get_offset( fd, &offset );
    return this_->pwrite(this_, fd, buf, nbyte, offset);
}

static ssize_t mem_pread(struct MountsPublicInterface* this_, 
			 int fd, void *buf, size_t nbyte, off_t offset){
    int ret;
    ino_t inode;
    int flags;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);

    HALLOCATOR_BY_MOUNT(this_)->get_flags( fd, &flags );

    /*check if file was not opened for reading*/
    CHECK_FILE_OPEN_FLAGS_OR_RAISE_ERROR(flags&O_ACCMODE, O_RDONLY, O_RDWR);

    ssize_t readed = MEMOUNT_BY_MOUNT(this_)->Read( inode, offset, buf, nbyte );
    if ( readed >= 0 ){
	offset += readed;
	/*update offset*/
	ret = HALLOCATOR_BY_MOUNT(this_)->set_offset( fd, offset );
	assert( ret == 0 );
    }
    /*return readed bytes or error*/
    return readed;
}

static ssize_t mem_pwrite(struct MountsPublicInterface* this_, 
			  int fd, const void *buf, size_t nbyte, off_t offset){
    int ret;
    ino_t inode;
    int flags;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);

    HALLOCATOR_BY_MOUNT(this_)->get_flags( fd, &flags );

    /*check if file was not opened for writing*/
    CHECK_FILE_OPEN_FLAGS_OR_RAISE_ERROR(flags&O_ACCMODE, O_WRONLY, O_RDWR);

    ssize_t wrote = MEMOUNT_BY_MOUNT(this_)->Write( inode, offset, buf, nbyte );
    if ( wrote != -1 ){
	offset += wrote;
	ret = HALLOCATOR_BY_MOUNT(this_)->set_offset( fd, offset );
	assert( ret == 0 );
    }
    return wrote;
}

static int mem_fchown(struct MountsPublicInterface* this_, int fd, uid_t owner, gid_t group){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);
    return MEMOUNT_BY_MOUNT(this_)->Chown( inode, owner, group);
}

static int mem_fchmod(struct MountsPublicInterface* this_, int fd, uint32_t mode){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);
    return MEMOUNT_BY_MOUNT(this_)->Chmod( inode, mode);
}


static int mem_fstat(struct MountsPublicInterface* this_, int fd, struct stat *buf){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);
    return MEMOUNT_BY_MOUNT(this_)->Stat( inode, buf);
}

static int mem_getdents(struct MountsPublicInterface* this_, int fd, void *buf, unsigned int count){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);

    off_t offset;
    int ret = HALLOCATOR_BY_MOUNT(this_)->get_offset( fd, &offset );
    assert( ret == 0 );
    ssize_t readed = MEMOUNT_BY_MOUNT(this_)->Getdents( inode, offset, (DIRENT*)buf, count);
    if ( readed != -1 ){
	offset += readed;
	ret = HALLOCATOR_BY_MOUNT(this_)->set_offset( fd, offset );
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
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);
    MemNode* mnode = NODE_OBJECT_BYINODE( MEMOUNT_BY_MOUNT(this_), inode);
    assert(mnode);

    MEMOUNT_BY_MOUNT(this_)->Unref(mnode->slot()); /*decrement use count*/
    if ( mnode->UnlinkisTrying() ){
	int ret = MEMOUNT_BY_MOUNT(this_)->UnlinkInternal(mnode);
	assert( ret == 0 );	
    }
    
    int ret = HALLOCATOR_BY_MOUNT(this_)->free_handle(fd);
    assert( ret == 0 );
    return 0;
}

static off_t mem_lseek(struct MountsPublicInterface* this_, int fd, off_t offset, int whence){
    ino_t inode;
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);

    if ( !is_dir(this_, inode) ){
	off_t next;
	int ret = HALLOCATOR_BY_MOUNT(this_)->get_offset(fd, &next );
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
	    len = get_file_len( this_, inode );
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
	ret = HALLOCATOR_BY_MOUNT(this_)->set_offset(fd, next );
	assert( ret == 0 );
	return next;
    }
    else{
	SET_ERRNO(EBADF);
	return -1;
    }
}

static int mem_open(struct MountsPublicInterface* this_, const char* path, int oflag, uint32_t mode){
    lazy_mount(this_, path);

    int ret=0;
    if (oflag & O_CREAT) {
	/*If need to create path, then do it with errors checking*/
	ret = MEMOUNT_BY_MOUNT(this_)->Open(path, oflag, mode);
    }

    /* get node from memory FS for specified type, if no errors occured 
     * then file allocated in memory FS and require file desc - fd*/
    struct stat st;
    if ( ret == 0 && MEMOUNT_BY_MOUNT(this_)->GetNode( path, &st) == 0 ){
	/*ask for file descriptor in handle allocator*/
	int fd = HALLOCATOR_BY_MOUNT(this_)->allocate_handle( this_ );
	if ( fd < 0 ){
	    /*it's hipotetical but possible case if amount of open files 
	      are exceeded an maximum value.*/
	    /*TODO: Add functional test for it*/
	    SET_ERRNO(ENFILE);
	    return -1;
	}
	
	/* As inode and fd are depend each form other, we need update 
	 * inode in handle allocator and stay them linked*/
	ret = HALLOCATOR_BY_MOUNT(this_)->set_inode( fd, st.st_ino );
	assert( ret == 0 );
	HALLOCATOR_BY_MOUNT(this_)->set_flags(fd, oflag);
	MEMOUNT_BY_MOUNT(this_)->Ref(st.st_ino); /*set file referred*/

	/*append feature support, is simple*/
	if ( oflag & O_APPEND ){
	    ZRT_LOG(L_SHORT, P_TEXT, "handle flag: O_APPEND");
	    mem_lseek(this_, fd, 0, SEEK_END);
	}

	/*file truncate support, only for writable files, reset size*/
	if ( oflag&O_TRUNC && (oflag&O_RDWR || oflag&O_WRONLY) ){
	    /*reset file size*/
	    MemNode* mnode = NODE_OBJECT_BYINODE( MEMOUNT_BY_MOUNT(this_), st.st_ino);
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
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);
    if ( is_dir(this_, inode) ){
	SET_ERRNO(EBADF);
	return -1;
    }
    return 0;
}

static int mem_remove(struct MountsPublicInterface* this_, const char* path){
    return MEMOUNT_BY_MOUNT(this_)->Unlink(path);
}

static int mem_unlink(struct MountsPublicInterface* this_, const char* path){
    return MEMOUNT_BY_MOUNT(this_)->Unlink(path);
}

static int mem_access(struct MountsPublicInterface* this_, const char* path, int amode){
    return -1;
}

static int mem_ftruncate_size(struct MountsPublicInterface* this_, int fd, off_t length){
    ino_t inode;
    MemNode* node; 
    GET_INODE_BY_HANDLE_OR_RAISE_ERROR( HALLOCATOR_BY_MOUNT(this_), fd, &inode);
    if ( !is_dir(this_, inode) ){
	if ((node=NODE_OBJECT_BYINODE( MEMOUNT_BY_MOUNT(this_), inode )) != NULL){

	    int flags;
	    HALLOCATOR_BY_MOUNT(this_)->get_flags( fd, &flags );

	    /*check if file was not opened for writing*/
	    flags &= O_ACCMODE;
	    if ( flags!=O_WRONLY && flags!=O_RDWR ){
		ZRT_LOG(L_ERROR, 
			"file open flags=%s not allow write", 
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
	    int ret = HALLOCATOR_BY_MOUNT(this_)->get_offset(fd, &offset );
	    assert(ret==0);
	    if ( length < offset ){
		offset = length+1;
		ret = HALLOCATOR_BY_MOUNT(this_)->set_offset(fd, offset );
		assert( ret == 0 );
	    }
#endif //DO_NOT_ALLOW_OFFSET_BEYOND_FILE_BOUNDS_IF_TRUNCATE_REDUCES_FILE_SIZE

	    ZRT_LOG(L_SHORT, "file truncated on %d len, updated st.size=%d",
		    (int)length, get_file_len( this_, inode ));
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
    lazy_mount(this_, oldpath);
    lazy_mount(this_, newpath);
    /*create new hardlink*/
    int ret = MEMOUNT_BY_MOUNT(this_)->Link(oldpath, newpath);
    if ( ret == -1 ){
	/*errno already setted by MemMount*/
	return ret;
    }

    /*ask for file descriptor in handle allocator*/
    int fd = HALLOCATOR_BY_MOUNT(this_)->allocate_handle( this_ );
    if ( fd < 0 ){
	/*it's hipotetical but possible case if amount of open files 
	  are exceeded an maximum value*/
	SET_ERRNO(ENFILE);
	return -1;
    }

    MemNode* mnode = NODE_OBJECT_BYPATH( MEMOUNT_BY_MOUNT(this_), newpath);
    assert(mnode);

    /* As inode and fd are depend each form other, we need update 
     * inode in handle allocator and stay them linked*/
    ret = HALLOCATOR_BY_MOUNT(this_)->set_inode( fd, mnode->slot() );
    ZRT_LOG(L_EXTRA, "errcode ret=%d", ret );
    assert( ret == 0 );

    return 0;
}

struct MountSpecificPublicInterface* mem_implem(struct MountsPublicInterface* this_){
    return ((struct InMemoryMounts*)this_)->mount_specific_interface;
}

static struct MountsPublicInterface KInMemoryMountWraper = {
    mem_chown,
    mem_chmod,
    mem_stat,
    mem_mkdir,
    mem_rmdir,
    mem_umount,
    mem_mount,
    mem_read,
    mem_write,
    mem_pread,
    mem_pwrite,
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
    mem_implem  /*mount_specific_interface*/
};

struct MountsPublicInterface* 
inmemory_filesystem_construct( struct HandleAllocator* handle_allocator ){
    /*use malloc and not new, because it's external c object*/
    struct InMemoryMounts* this_ = (struct InMemoryMounts*)malloc( sizeof(struct InMemoryMounts) );

    /*set functions*/
    this_->public_ = KInMemoryMountWraper;
    /*set data members*/
    this_->handle_allocator = handle_allocator; /*use existing handle allocator*/
    this_->mount_specific_interface = CONSTRUCT_L(MOUNT_SPECIFIC)( &KMountSpecificImplem,
								   this_);
    this_->mem_mount_cpp = new MemMount;
    return (struct MountsPublicInterface*)this_;
}

