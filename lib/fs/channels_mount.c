/*
 * channels_mount.c
 *
 *  Created on: 19.09.2012
 *      Author: yaroslav
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>  //SEEK_SET .. SEEK_END
#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "zvm.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "nacl_struct.h"
#include "mount_specific_implem.h"
#include "handle_allocator.h"
#include "fcntl_implem.h"
#include "enum_strings.h"
#include "channels_readdir.h"
#include "channels_mount.h"

enum PosAccess{ EPosSeek=0, EPosRead, EPosWrite };
enum PosWhence{ EPosGet=0, EPosSetAbsolute, EPosSetRelative };

struct ZrtChannelRt{
    int     handle;
    int     flags;                 /*For currently opened file contains flags*/
    int64_t sequential_access_pos; /*sequential read, sequential write*/
    int64_t random_access_pos;     /*random read, random write*/
    int64_t maxsize;               /*synthethic size. based on maximum position of cursor pos 
				     channel for all I/O requests*/
    struct flock fcntl_flock;      /*lock flag for support fcntl locking function*/
    int     fcntl_flags;           /*F_GETFD, F_SETFD support for fcntl*/
};

/*0 if check OK*/
#define CHECK_NEW_POS(offset) ((offset) < 0 ? -1: 0)
#define SET_SAFE_OFFSET( whence, pos_p, offset )	\
    if ( EPosSetRelative == whence ){			\
	if ( CHECK_NEW_POS( *pos_p+offset ) == -1 )	\
            { SET_ERRNO( EOVERFLOW ); return -1; }	\
	else return *pos_p +=offset; }			\
    else{						\
	if ( CHECK_NEW_POS( offset ) == -1 )		\
	    { SET_ERRNO( EOVERFLOW ); return -1; }	\
	else return *pos_p  =offset; }



#define CHANNEL_SIZE(handle) s_zrt_channels[handle] ?			\
    MAX( s_zrt_channels[handle]->maxsize, s_channels_list[handle].size ) : s_channels_list[handle].size;

#define CHANNEL_ASSERT_IF_FAIL( handle )  assert( handle >=0 && handle < s_channels_count )


//////////////// data
struct HandleAllocator* s_handle_allocator;
struct manifest_loaded_directories_t s_manifest_dirs;
/*runtime information related to channels*/
static struct ZrtChannelRt**  s_zrt_channels;
static const struct ZVMChannel* s_channels_list;
static int s_channels_count;


/* Channels specific implementation functions intended to initialize
 * mount_specific_implem struct pointers;
 * */

/*return 0 if handle not valid, or 1 if handle is correct*/
static int check_handle(int handle){
    return (handle>=0 && handle < s_channels_count) ? 1 : 0;
}

/*return pointer at success, NULL if fd didn't found or flock structure has not been set*/
static const struct flock* flock_data( int fd ){
    const struct flock* data = NULL;
    if ( check_handle(fd) ){
	/*get runtime information related to channel*/
	struct ZrtChannelRt *zrt_channel = s_zrt_channels[fd];
	data = &zrt_channel->fcntl_flock;
    }
    return data;
}

/*return 0 if success, -1 if fd didn't found*/
static int set_flock_data( int fd, const struct flock* flock_data ){
    int rc = 1; /*error by default*/
    if ( check_handle(fd) ){
	/*get runtime information related to channel*/
	struct ZrtChannelRt *zrt_channel = s_zrt_channels[fd];
	memcpy(&zrt_channel->fcntl_flock, flock_data, sizeof(struct flock));
	rc = 0; /*ok*/
    }
    return rc;
}

static struct mount_specific_implem s_mount_specific_implem = {
    check_handle,
    flock_data,
    set_flock_data
};


//////////// helpers

static int channel_handle(const char* path){
    const struct ZVMChannel* chan = NULL;

    /* search for name through the channels list*/
    int handle;
    for(handle = 0; handle < s_channels_count; ++handle)
        if(strcmp(s_channels_list[handle].name, path) == 0){
            chan = &s_channels_list[handle];
            break; /*handle according to matched filename*/
        }

    if ( !chan )
        return -1; /* if channel name not matched return error*/
    else
        return handle;
}

/*return 0 if specified mode is matches to chan AccessType*/
static int check_channel_access_mode(const struct ZVMChannel *chan, int access_mode)
{
    assert(chan);

    /*check read / write accasebility*/
    int canberead = chan->limits[GetsLimit] && chan->limits[GetSizeLimit];
    int canbewrite = chan->limits[PutsLimit] && chan->limits[PutSizeLimit];

    ZRT_LOG(L_INFO, "access_mode=%u, canberead=%d, canbewrite=%d", 
	    access_mode, canberead, canbewrite );

    /*reset permissions bits, that are not used currently*/
    access_mode = access_mode & O_ACCMODE;
    switch( access_mode ){
    case O_RDONLY:
        return canberead>0 ? 0: -1;
    case O_WRONLY:
        return canbewrite >0 ? 0 : -1;
    case O_RDWR:
    default:
        /*if case1: O_RDWR ; case2: O_RDWR|O_WRONLY logic error handle as O_RDWR*/
        return canberead>0 && canbewrite>0 ? 0 : -1;
    }
    return 1;
}

static void debug_mes_zrt_channel_runtime( int handle ){
    if ( handle == 0 ) return;
    const struct ZrtChannelRt *zrt_chan_runtime = s_zrt_channels[handle];
    ZRT_LOG(L_INFO, "handle=%d", handle);
    if (zrt_chan_runtime){
        ZRT_LOG(L_INFO, 
		"flags=%s, sequential_access_pos=%llu, random_access_pos=%llu",
                FILE_OPEN_FLAGS(zrt_chan_runtime->flags),
                zrt_chan_runtime->sequential_access_pos, zrt_chan_runtime->random_access_pos );
    }
}


static int open_channel( const char *name, int flags, int mode )
{
    int handle = channel_handle(name);
    ZRT_LOG(L_EXTRA, 
	    "name=%s, handle=%d, mode=%s, flags=%s", 
	    name, handle, FILE_OPEN_MODE(mode), FILE_OPEN_FLAGS(flags) );
    const struct ZVMChannel *chan = NULL;

    if ( handle != -1 ){
        CHANNEL_ASSERT_IF_FAIL( handle );
        chan = &s_channels_list[handle];
    }
    else{
        /* channel name not matched return error*/
        SET_ERRNO( ENOENT );
        return -1;
    }

    /*return handle if file already opened*/
    if ( s_zrt_channels[handle] && s_zrt_channels[handle]->flags >= 0 ){
        ZRT_LOG(L_ERROR, "channel already opened, handle=%d ", handle );
        return handle;
    }

    /*check access mode for opening channel, limits not checked*/
    if( check_channel_access_mode( chan, flags ) != 0 ){
        ZRT_LOG(L_ERROR, "can't open channel, handle=%d ", handle );
        SET_ERRNO( EACCES );
        return -1;
    }

    /*alloc zrt runtime channel object or update existing*/
    if ( !s_zrt_channels[handle] ){
        s_zrt_channels[handle] = calloc( 1, sizeof(struct ZrtChannelRt) );
    }
    s_zrt_channels[handle]->flags = flags;

#ifdef DEBUG
    debug_mes_zrt_channel_runtime(handle);
#endif
    ZRT_LOG(L_EXTRA, "channel open ok, handle=%d ", handle );
    return handle;
}


static uint32_t channel_permissions(const struct ZVMChannel *channel){
    uint32_t perm = 0;
    assert(channel);
    if ( channel->limits[GetsLimit] != 0 && channel->limits[GetSizeLimit] )
        perm |= S_IRUSR;
    if ( channel->limits[PutsLimit] != 0 && channel->limits[PutSizeLimit] )
        perm |= S_IWUSR;
    if ( (channel->type == SGetSPut && ( (perm&S_IRWXU)==S_IRUSR || (perm&S_IRWXU)==S_IWUSR )) ||
	 (channel->type == RGetSPut && (perm&S_IRWXU)==S_IWUSR ) ||
	 (channel->type == SGetRPut && (perm&S_IRWXU)==S_IRUSR ) )
	{
	    perm |= S_IFCHR;
	}
    else{
        perm |= S_IFBLK;
    }
    return perm;
}


static int64_t channel_pos_sequen_get_sequen_put( struct ZrtChannelRt *zrt_channel,
						  int8_t whence, int8_t access, int64_t offset ){
    /*seek get supported by channel for any modes*/
    if ( EPosGet == whence ) return zrt_channel->sequential_access_pos;
    switch ( access ){
    case EPosSeek:
        /*seek set does not supported for this channel type*/
        SET_ERRNO( ESPIPE );
        return -1;
    case EPosRead:
    case EPosWrite:
        SET_SAFE_OFFSET( whence, &zrt_channel->sequential_access_pos, offset );
        break;
    default:
        assert(0);
        break;
    }
}


static int64_t channel_pos_random_get_sequen_put( struct ZrtChannelRt *zrt_channel,
						  int8_t whence, int8_t access, int64_t offset ){
    switch ( access ){
    case EPosSeek:
        if ( CHECK_FLAG( zrt_channel->flags, O_RDONLY )
	     || CHECK_FLAG( zrt_channel->flags, O_RDWR ) )
	    {
		if ( EPosGet == whence ) return zrt_channel->random_access_pos;
		SET_SAFE_OFFSET( whence, &zrt_channel->random_access_pos, offset );
	    }
        else if ( CHECK_FLAG( zrt_channel->flags, O_WRONLY ) == 1 )
	    {
		if ( EPosGet == whence ) return zrt_channel->sequential_access_pos;
		else{
		    /*it's does not supported for seeking sequential write*/
		    SET_ERRNO( ESPIPE );
		    return -1;
		}
	    }
        break;
    case EPosRead:
        if ( EPosGet == whence ) return zrt_channel->random_access_pos;
        SET_SAFE_OFFSET( whence, &zrt_channel->random_access_pos, offset );
        break;
    case EPosWrite:
        if ( EPosGet == whence ) return zrt_channel->sequential_access_pos;
        SET_SAFE_OFFSET( whence, &zrt_channel->sequential_access_pos, offset );
        break;
    default:
        assert(0);
        break;
    }
}

static int64_t channel_pos_sequen_get_random_put(struct ZrtChannelRt *zrt_channel,
						 int8_t whence, int8_t access, int64_t offset){
    switch ( access ){
    case EPosSeek:
        if ( CHECK_FLAG( zrt_channel->flags, O_WRONLY ) == 1
	     || CHECK_FLAG( zrt_channel->flags, O_RDWR ) == 1 )
	    {
		if ( EPosGet == whence ) return zrt_channel->random_access_pos;
		SET_SAFE_OFFSET( whence, &zrt_channel->random_access_pos, offset );
	    }
        else if ( CHECK_FLAG( zrt_channel->flags, O_RDONLY )  == 1)
	    {
		if ( EPosGet == whence ) return zrt_channel->sequential_access_pos;
		else{
		    /*it's does not supported for seeking sequential read*/
		    SET_ERRNO( ESPIPE );
		    return -1;
		}
	    }
        break;
    case EPosRead:
        if ( EPosGet == whence ) return zrt_channel->sequential_access_pos;
        SET_SAFE_OFFSET( whence, &zrt_channel->sequential_access_pos, offset );
        break;
    case EPosWrite:
        if ( EPosGet == whence ) return zrt_channel->random_access_pos;
        SET_SAFE_OFFSET( whence, &zrt_channel->random_access_pos, offset );
        break;
    default:
        assert(0);
        break;
    }
}

static int64_t channel_pos_random_get_random_put(struct ZrtChannelRt *zrt_channel,
						 int8_t whence, int8_t access, int64_t offset){
    if ( EPosGet == whence ) return zrt_channel->random_access_pos;
    switch ( access ){
    case EPosSeek:
    case EPosRead:
    case EPosWrite:
        SET_SAFE_OFFSET( whence, &zrt_channel->random_access_pos, offset );
        break;
    default:
        assert(0);
        break;
    }
}

/*@param pos_whence If EPosGet offset unused, otherwise check and set offset
 *@return -1 if bad offset, else offset result*/
static int64_t channel_pos( int handle, int8_t whence, int8_t access, int64_t offset ){
    if ( check_handle(handle) != 0 && s_zrt_channels[handle] ){
        struct ZrtChannelRt *zrt_channel = s_zrt_channels[handle];
        assert( zrt_channel );
        CHANNEL_ASSERT_IF_FAIL( handle );
        int8_t access_type = s_channels_list[handle].type;
        switch ( access_type ){
        case SGetSPut:
            return channel_pos_sequen_get_sequen_put( zrt_channel, whence, access, offset );
            break;
        case RGetSPut:
            return channel_pos_random_get_sequen_put( zrt_channel, whence, access, offset );
            break;
        case SGetRPut:
            return channel_pos_sequen_get_random_put( zrt_channel, whence, access, offset );
            break;
        case RGetRPut:
            return channel_pos_random_get_random_put( zrt_channel, whence, access, offset );
            break;
        }
    }
    else{
        /*bad handle*/
        SET_ERRNO( EBADF );
    }
    return -1;
}

static void set_stat_timestamp( struct stat* st )
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    /* files does not have real date/time */
    st->st_atime = tv.tv_sec;      /* time of the last access */
    st->st_mtime = tv.tv_sec;      /* time of the last modification */
    st->st_ctime = tv.tv_sec;      /* time of the last status change */
}

/*used by stat, fstat; set stat based on channel type*/
static void set_stat(struct stat *stat, int fd)
{
    ZRT_LOG(L_EXTRA, "fd=%d", fd);

    int nlink = 1;
    uint32_t permissions = 0;
    int64_t size=4096;
    uint32_t blksize =1;
    uint64_t ino;

    /*choose handle type: channel handle or dir handle */
    if ( check_handle(fd) ){
        /*channel handle*/
        CHANNEL_ASSERT_IF_FAIL( fd );
        permissions = channel_permissions( &s_channels_list[fd] );
        if ( CHECK_FLAG( permissions, S_IFCHR) ) blksize = 1;
        else                                     blksize = 4096;
        ino = INODE_FROM_HANDLE( fd );
        size = CHANNEL_SIZE(fd);
    }
    else{
        /*dir handle*/
        struct dir_data_t *d = match_handle_in_directory_list( &s_manifest_dirs, fd );
        assert(d);
        nlink = d->nlink;
        ino = INODE_FROM_HANDLE(d->handle);
        permissions |= S_IRUSR | S_IFDIR;
    }

    /* return stat object */
    stat->st_dev = 2049;     /* ID of device containing handle */
    stat->st_ino = ino;     /* inode number */
    stat->st_nlink = nlink;      /* number of hard links */;
    stat->st_uid = 1000;     /* user ID of owner */
    stat->st_gid = 1000;     /* group ID of owner */
    stat->st_rdev = 0;       /* device ID (if special handle) */
    stat->st_mode = permissions;
    stat->st_blksize = blksize;        /* block size for file system I/O */
    stat->st_size = size;
    stat->st_blocks =               /* number of 512B blocks allocated */
	((stat->st_size + stat->st_blksize - 1) / stat->st_blksize) * stat->st_blksize / 512;

    set_stat_timestamp( stat );
}


/*@return 0 if matched, or -1 if not*/
int iterate_dir_contents( int dir_handle, int index, 
			  int* iter_fd, const char** iter_name, int* iter_is_dir ){
    int i=0;

    /*get directory data by handle*/
    struct dir_data_t* dir_pattern =
	match_handle_in_directory_list(&s_manifest_dirs, dir_handle);

    /*nested dirs & files count*/
    int subdir_index = 0;
    int file_index = 0;

    /*match subdirs*/
    struct dir_data_t* loop_d = NULL; /*loop dir used as loop variable*/
    int namelen =  strlen(dir_pattern->path);
    for( i=0; i < s_manifest_dirs.dircount; i++ ){
	/*iterate every directory to compare it with pattern*/
        loop_d = &s_manifest_dirs.dir_array[i];
        /*match all subsets, exclude the same dir path*/
        if ( ! strncmp(dir_pattern->path, loop_d->path, namelen) && 
	     strlen(loop_d->path) > namelen+1 ){
            /*if can't locate trailing '/' then matched subdir*/
            char *backslash_matched = strchr( &loop_d->path[namelen+1], '/');
            if ( !backslash_matched ){
		/*if matched index of directory contents*/
		if ( index == subdir_index ){
		    /*fetch name from full path*/
		    int shortname_len;
		    const char *short_name = 
			name_from_path_get_path_len(loop_d->path, &shortname_len );
		    *iter_fd = loop_d->handle; /*get directory handle*/
		    *iter_name = short_name; /*get name of item*/
		    *iter_is_dir = 1; /*get info that item is directory*/
		    return 0;
		}
		++subdir_index;
            }
        }
    }

    /*match files*/
    int dirlen =  strlen(dir_pattern->path);
    for( i=0; i < s_channels_count; i++ ){
        /*match all subsets for dir path*/
        if ( ! strncmp(dir_pattern->path, s_channels_list[i].name, dirlen) && 
	     strlen(s_channels_list[i].name) > dirlen+1 ){
            /*if can't locate trailing '/' then matched directory file*/
            char *backslash_matched = strchr( &s_channels_list[i].name[dirlen+1], '/');
            if ( !backslash_matched ){
		/*if matched index of directory contents*/
		if ( index == file_index ){
		    /*fetch name from full path*/
		    int shortname_len;
		    const char *short_name = 
			name_from_path_get_path_len(s_channels_list[i].name, &shortname_len );
		    *iter_fd = i; /*channel handle is the same as channel index*/
		    *iter_name = short_name; /*get name of item*/
		    *iter_is_dir = 0; /*get info that item is not directory*/
		    return 0;
		}
		++file_index;
	    }
        }
    }
    return -1;/*specified index not matched, probabbly it's out of bounds*/
}



//////////// interface implementation

static int channels_chmod(const char* path, uint32_t mode){
    SET_ERRNO(EPERM);
    return -1;
}

static int channels_stat(const char* path, struct stat *buf){
    errno = 0;
    ZRT_LOG(L_EXTRA, "path=%s", path);
    struct stat *sbuf = (struct stat *)buf;

    if(path == NULL){
        SET_ERRNO(EFAULT);
        return -1;
    }

    struct dir_data_t *dir = NULL;
    int handle = channel_handle(path);
    if ( handle != -1 ){
        set_stat( sbuf, handle);
        return 0;
    }
    else if ( (dir=match_dir_in_directory_list(&s_manifest_dirs, path, strlen(path))) ){
        set_stat( sbuf, dir->handle);
        return 0;
    }
    else{
        SET_ERRNO(ENOENT);
        return -1;
    }

    return 0;
}

static int channels_mkdir(const char* path, uint32_t mode){
    SET_ERRNO(ENOSYS);
    return -1;
}

static int channels_rmdir(const char* path){
    SET_ERRNO(ENOSYS);
    return -1;
}

static int channels_umount(const char* path){
    SET_ERRNO(ENOSYS);
    return -1;
}

static int channels_mount(const char* path, void *mount){
    SET_ERRNO(ENOSYS);
    return -1;
}

static ssize_t channels_read(int fd, void *buf, size_t nbyte){
    errno = 0;
    int32_t readed = 0;

    /*case: file not opened, bad descriptor*/
    if ( check_handle(fd) == 0  ||
	 !s_zrt_channels[fd] ||
	 s_zrt_channels[fd]->flags < 0 )
	{
	    ZRT_LOG(L_ERROR, "invalid file descriptor fd=%d", fd);
	    SET_ERRNO( EBADF );
	    return -1;
	}
    CHANNEL_ASSERT_IF_FAIL(fd);
    ZRT_LOG(L_INFO, "channel name=%s", s_channels_list[fd].name );
    debug_mes_zrt_channel_runtime( fd );

    /*check if file was not opened for reading*/
    int flags= s_zrt_channels[fd]->flags;
    if ( !CHECK_FLAG(flags, O_RDONLY) && 
	 !CHECK_FLAG(flags, O_RDWR)  )
	{
	    ZRT_LOG(L_ERROR, "file open_mode=%s not allowed read", FILE_OPEN_FLAGS(flags));
	    SET_ERRNO( EINVAL );
	    return -1;
	}

    int64_t pos = channel_pos(fd, EPosGet, EPosRead, 0);
    if ( CHECK_NEW_POS( pos+nbyte ) != 0 ){
        ZRT_LOG(L_ERROR, "file bad pos=%lld", pos);
        SET_ERRNO( EOVERFLOW );
        return -1;
    }

    readed = zvm_pread(fd, buf, nbyte, pos );
    if(readed > 0) channel_pos(fd, EPosSetRelative, EPosRead, readed);

    ZRT_LOG(L_EXTRA, "channel fd=%d, bytes readed=%d", fd, readed);

    if ( readed < 0 ){
        SET_ERRNO( zvm_errno() );
        return -1;
    }

    return readed;
}

static ssize_t channels_write(int fd, const void *buf, size_t nbyte){
    int32_t wrote = 0;

    //if ( fd < 3 ) disable_logging_current_syscall();

    /*file not opened, bad descriptor*/
    if ( check_handle(fd) == 0 ||
	 !s_zrt_channels[fd] ||
	 s_zrt_channels[fd]->flags < 0  )
	{
	    ZRT_LOG(L_ERROR, "invalid file descriptor fd=%d", fd);
	    SET_ERRNO( EBADF );
	    return -1;
	}

    CHANNEL_ASSERT_IF_FAIL(fd);
    ZRT_LOG( L_EXTRA, "channel name=%s", s_channels_list[fd].name );
    debug_mes_zrt_channel_runtime( fd );

    /*if file was not opened for writing, set errno and get error*/
    int flags= s_zrt_channels[fd]->flags;
    if ( !CHECK_FLAG(flags, O_WRONLY) && 
	 !CHECK_FLAG(flags, O_RDWR)  )
	{
	    ZRT_LOG(L_ERROR, "file open_mode=%s not allowed write", FILE_OPEN_FLAGS(flags));
	    SET_ERRNO( EINVAL );
	    return -1;
	}

    int64_t pos = channel_pos(fd, EPosGet, EPosWrite, 0);
    if ( CHECK_NEW_POS( pos+nbyte ) != 0 ){
        SET_ERRNO( EOVERFLOW );
        return -1;
    }

    wrote = zvm_pwrite(fd, buf, nbyte, pos );
    if(wrote > 0) channel_pos(fd, EPosSetRelative, EPosWrite, wrote);
    ZRT_LOG(L_EXTRA, "channel fd=%d, bytes wrote=%d", fd, wrote);

    if ( wrote < 0 ){
        SET_ERRNO( zvm_errno() );
        return -1;
    }

    /*save maximum writable position for random access write only channels using 
      this as synthetic size for channel mapped file. Here is works a single rule: 
      maximum writable position can be used as size*/
    CHANNEL_ASSERT_IF_FAIL(fd);
    int8_t access_type = s_channels_list[fd].type;
    if ( CHECK_FLAG(s_zrt_channels[fd]->flags, O_WRONLY ) &&
	 (access_type == SGetRPut || access_type == RGetRPut) )
	{
	    s_zrt_channels[fd]->maxsize = channel_pos(fd, EPosGet, EPosWrite, 0);
	    ZRT_LOG(L_EXTRA, "Set channel size=%lld", s_zrt_channels[fd]->maxsize );
	}

    return wrote;
}

static int channels_fchmod(int fd, mode_t mode){
    SET_ERRNO(EPERM);
    return -1;
}

static int channels_fstat(int fd, struct stat *buf){
    errno = 0;
    ZRT_LOG(L_EXTRA, "fd=%d", fd);

    /* check if user request contain the proper file fd */
    if( check_handle(fd) != 0 ){
        set_stat( buf, fd); /*channel fd*/
    }
    else if ( match_handle_in_directory_list(&s_manifest_dirs, fd) ){
        set_stat( buf, fd); /*dir fd*/
    }
    else{
        SET_ERRNO(EBADF);
        return -1;
    }

    return 0;
}

static int channels_getdents(int fd, void *buf, unsigned int buf_size){
    errno=0;
    ZRT_LOG( L_INFO, "fd=%d, buf_size=%d", fd, buf_size );

    /* check null and  make sure the buffer is aligned */
    if ( !buf || 0 != ((sizeof(unsigned long) - 1) & (uintptr_t) buf)) {
        SET_ERRNO(EINVAL);
        return -1;
    }
    int bytes_read=0;

    /*get offset, and use it as cursor that using to iterate directory contents*/
    off_t offset;
    s_handle_allocator->get_offset(fd, &offset );

    /*through via list all directory contents*/
    int index=offset;
    int iter_fd=0;
    const char* iter_item_name = NULL;
    int iter_is_dir=0;
    int res=0;
    while( !(res=iterate_dir_contents( fd, index, &iter_fd, &iter_item_name, &iter_is_dir )) ){
	/*format in buf dirent structure, of variable size, and save current file data;
	  original MemMount implementation was used dirent as having constant size */
	int ret = 
	    put_dirent_into_buf( ((char*)buf)+bytes_read, buf_size-bytes_read, 
				 INODE_FROM_HANDLE(iter_fd), 0,
				 iter_item_name, strlen(iter_item_name) );
	/*if put into dirent was success*/
	if ( ret > 0 ){
	    bytes_read += ret;
	    ++index;
	}
	else{
	    break; /*interrupt - insufficient buffer space*/
	}
    }
    /*update offset index of handled directory item*/
    offset = index;
    s_handle_allocator->set_offset(fd, offset );
    return bytes_read;
}

static int channels_fsync(int fd){
    SET_ERRNO(ENOSYS);
    return -1;
}

static int channels_close(int fd){
    errno = 0;
    ZRT_LOG(L_EXTRA, "fd=%d", fd);

    /*if valid fd and file was opened previously then perform file close*/
    if ( check_handle(fd) != 0 &&
	 s_zrt_channels[fd] && s_zrt_channels[fd]->flags >=0  )
	{
	    s_zrt_channels[fd]->random_access_pos 
		= s_zrt_channels[fd]->sequential_access_pos = 0;
	    s_zrt_channels[fd]->maxsize = 0;
	    s_zrt_channels[fd]->flags = -1;
	    ZRT_LOG(L_EXTRA, "closed channel=%s", s_channels_list[fd].name );
	    //zvm_close(fd);
	    return 0;
	}
    else{ /*search fd in directories list*/
        int i;
        for ( i=0; i < s_manifest_dirs.dircount; i++ ){
            /*if matched fd*/
            if ( s_manifest_dirs.dir_array[i].handle == fd &&
		 s_manifest_dirs.dir_array[i].flags >= 0 )
		{
		    /*close opened dir*/
		    s_manifest_dirs.dir_array[i].flags = -1;
		    /*reset offset of last directory i/o operations*/
		    off_t offset = 0;
		    s_handle_allocator->set_offset(fd, offset );
		    return 0;
		}
        }
        /*no matched open dir fd*/
        SET_ERRNO( EBADF );
        return -1;
    }

    return 0;
}

static off_t channels_lseek(int fd, off_t offset, int whence){
    errno = 0;
    ZRT_LOG(L_INFO, "offt size=%d, fd=%d, offset=%lld", sizeof(off_t), fd, offset);
    debug_mes_zrt_channel_runtime( fd );

    /* check fd */
    if( check_handle(fd) == 0 ){
        SET_ERRNO( EBADF );
        return -1;
    }
    CHANNEL_ASSERT_IF_FAIL(fd);
    ZRT_LOG(L_EXTRA, "channel name=%s, whence=%s", 
	    s_channels_list[fd].name, SEEK_WHENCE(whence) );

    switch(whence)
	{
	case SEEK_SET:
	    offset = channel_pos(fd, EPosSetAbsolute, EPosSeek, offset );
	    break;
	case SEEK_CUR:
	    if ( !offset )
		offset = channel_pos(fd, EPosGet, EPosSeek, offset );
	    else
		offset = channel_pos(fd, EPosSetRelative, EPosSeek, offset );
	    break;
	case SEEK_END:{
	    off_t size = CHANNEL_SIZE(fd);
	    /*use runtime size instead static size in zvm channel*/
	    offset = channel_pos(fd, EPosSetAbsolute, EPosSeek, size + offset );
	    break;
	}
	default:
	    SET_ERRNO( EPERM ); /* in advanced version should be set to conventional value */
	    return -1;
	}

    /*
     * return current position in a special way since 64 bits
     * doesn't fit to return code (32 bits)
     */
    return offset;
}

static int channels_open(const char* path, int oflag, uint32_t mode){
    errno=0;

    /*If specified open flag saying as that trying to open not directory*/
    if ( CHECK_FLAG(oflag, O_DIRECTORY) == 0 ){
        return open_channel( path, oflag, mode );
    }
    else { /*trying to open directory*/
        struct dir_data_t *dir = 
	    match_dir_in_directory_list( &s_manifest_dirs, path, strlen(path));
        /*if valid directory path matched */
        if ( dir != NULL ){
            if ( dir->flags < 0 ){ /*if not opened*/
                /*if trying open in read only*/
                if ( CHECK_FLAG(oflag, O_RDONLY) ){  /*it's allowed*/
                    dir->flags = oflag;
                    return dir->handle;
                }
                else{  /*Not allowed read-write / write access*/
                    SET_ERRNO( EACCES );
                    return -1;
                }
            }
            else { /*already opened, just return handle*/
                return dir->handle;
            }
        }
        else{
            /*no matched directory*/
            SET_ERRNO( ENOENT );
            return -1;
        }
    }
    return 0;
}

static int channels_fcntl(int fd, int cmd, ...){
    ZRT_LOG(L_INFO, "fcntl cmd=%s", FCNTL_CMD(cmd));

    int ret=0;
    va_list args;
    va_start(args, cmd);
    if ( cmd == F_SETLK || cmd == F_SETLKW || cmd == F_GETLK ){
	struct flock* input_lock = va_arg(args, struct flock*);
	ret = fcntl_implem(&s_mount_specific_implem, fd, cmd, input_lock);
    }
    va_end(args);

    return ret;
}

static int channels_remove(const char* path){
    SET_ERRNO( EPERM );
    return -1;
}

static int channels_unlink(const char* path){
    SET_ERRNO( EPERM );
    return -1;

}
// access() uses the Mount's Stat().
static int channels_access(const char* path, int amode){
    SET_ERRNO( ENOSYS );
    return -1;
}

static int channels_isatty(int fd){
    SET_ERRNO( ENOSYS );
    return -1;

}

static int channels_dup(int oldfd){
    SET_ERRNO( ENOSYS );
    return -1;
}

static int channels_dup2(int oldfd, int newfd){
    SET_ERRNO( ENOSYS );
    return -1;
}

static int channels_link(const char* path1, const char* path2){
    SET_ERRNO( ENOSYS );
    return -1;
}

static int channels_chown(const char* p, uid_t u, gid_t g){
    SET_ERRNO( ENOSYS );
    return -1;
}

static int channels_fchown(int f, uid_t u, gid_t g){
    SET_ERRNO( ENOSYS );
    return -1;
}

/*filesystem interface initialisation*/
static struct MountsInterface s_channels_mount = {
    channels_chown,
    channels_chmod,
    channels_stat,
    channels_mkdir,
    channels_rmdir,
    channels_umount,
    channels_mount,
    channels_read,
    channels_write,
    channels_fchown,
    channels_fchmod,
    channels_fstat,
    channels_getdents,
    channels_fsync,
    channels_close,
    channels_lseek,
    channels_open,
    channels_fcntl,
    channels_remove,
    channels_unlink,
    channels_access,
    channels_isatty,
    channels_dup,
    channels_dup2,
    channels_link,
    EChannelsMountId
};


struct MountsInterface* alloc_channels_mount( struct HandleAllocator* handle_allocator,
					      const struct ZVMChannel* channels, int channels_count ){
    s_handle_allocator = handle_allocator;
    s_channels_list = channels;
    s_channels_count = channels_count;
    /* array of channels runtime data. For opened channels suitable data is:
     * opened flags, mode, i/o positions*/
    s_zrt_channels = calloc( channels_count, sizeof(struct ZrtChannelRt*) );

    s_manifest_dirs.dircount=0;
    process_channels_create_dir_list( channels, channels_count, &s_manifest_dirs );

    /*allocate reserved handles that is unchanged*/
    if ( handle_allocator ){
        int i;
        /*reserve all handles used by channels*/
        for( i=0; i < channels_count; i++ ){
            int h = handle_allocator->allocate_reserved_handle( &s_channels_mount, i );
            assert( h == i ); /*allocated handle should be the same as requested*/
        }
        /*reserve all handles that uses dirs related to channels*/
        for ( i=0; i < s_manifest_dirs.dircount; i++ ){
            int handle = s_manifest_dirs.dir_array[i].handle;
            int h = handle_allocator->allocate_reserved_handle( &s_channels_mount, handle );
            assert( h == handle ); /*allocated handle should be the same as requested*/
        }
    }

    /*this info will not be logged here because logs not yet created */
#ifdef DEBUG
    ZRT_LOG(L_EXTRA, "dirs count loaded from manifest=%d", s_manifest_dirs.dircount);
    int i;
    for ( i=0; i < s_manifest_dirs.dircount; i++ ){
        ZRT_LOG( L_EXTRA, "dir[%d].handle=%d; .path=%s; .nlink=%d", i,
		 s_manifest_dirs.dir_array[i].handle,
		 s_manifest_dirs.dir_array[i].path,
		 s_manifest_dirs.dir_array[i].nlink);
    }
#endif

    return &s_channels_mount;
}


