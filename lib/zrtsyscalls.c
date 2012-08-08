/*
 * syscallbacks.c
 * Syscallbacks implementation used by zrt;
 *
 *  Created on: 6.07.2012
 *      Author: YaroslavLitvinov
 */

#define _GNU_SOURCE
#define __USE_LARGEFILE64
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <sys/types.h> //off_t
#include <sys/stat.h>
#include <string.h> //memcpy
#include <stdio.h>  //SEEK_SET .. SEEK_END
#include <stdlib.h> //atoi
#include <stddef.h> //size_t
#include <unistd.h> //STDIN_FILENO
#include <fcntl.h> //file flags, O_ACCMODE
#include <errno.h>
#include <dirent.h>     /* Defines DT_* constants */
#include <assert.h>

#include "zvm.h"
#include "zrt.h"
#include "zrtreaddir.h"
#include "zrtsyscalls.h"


/* TODO
 * RESEARCH
 * fstat / stat changes according to channel types
 * rename : ENOSYS 38 Function not implemented
 * CURRENTLY
 * fseek uses long offset
 * lseek uses off_t offset
 * ftell pos for std* streams
 * SUPPORT
 * fstat for /etc/localtime, set timestamp as localtime
 * big file size 5gb _FILE_OFFSET_BITS=64
 * use fpos64_t; fgetpos64, fsetpos64
 * * _FILE_OFFSET_BITS=32
 * use fpos_t; fgetpos, fsetpos
 * EOF all cases:
 * - zvm_pread return 0 bytes, errno 0;
 * - zvm_pread return err code, if limits reached;
 * - read - bytes read is returned (zero indicates end of file;
 * - pread - zero indicates that end of file
 * - test EOF for channel files: getchar, getline
 * - feof - returning nonzero if it is set to EOF
 * TESTS
 * : EOF correctly return by read, fread, feof,*
 * : channels close, reopen, reclose
 * : try to test fstat returned data
 * BUGS
 * : fdopen failed, ftell fread
 * */

/* ******************************************************************************
 * Syscallbacks debug macros*/
#ifdef DEBUG
#define ZRT_LOG_NAME "/dev/debug"
#define SHOWID zrt_log("%s", "syscall called")
#else
#define SHOWID
#endif

#define MAX(a,b) \
        ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a > _b ? _a : _b; })


/* ******************************************************************************
 * Syscallback mocks helper macroses*/
#define JOIN(x,y) x##y
#define ZRT_FUNC(x) JOIN(zrt_, x)

/* mock. replacing real syscall handler */
#define SYSCALL_MOCK(name_wo_zrt_prefix, code) \
        static int32_t ZRT_FUNC(name_wo_zrt_prefix)(uint32_t *args)\
        {\
    SHOWID;\
    return code;\
        }

/*0 if check OK*/
#define CHECK_NEW_POS(offset) ((offset) < 0 || (offset) > 0x7fffffff ? -1: 0)
#define CHECK_FLAG(flags, flag) ( (flags & flag) == flag? 1 : 0)
#define SET_SAFE_OFFSET( whence, pos_p, offset ) \
        if ( EPosSetRelative == whence ){ \
            if ( CHECK_NEW_POS( *pos_p+offset ) == -1 ) \
            { set_zrt_errno( EOVERFLOW ); return -1; } \
            else return *pos_p +=offset; } \
            else{ \
                if ( CHECK_NEW_POS( offset ) == -1 ) \
                { set_zrt_errno( EOVERFLOW ); return -1; } \
                else return *pos_p  =offset; }

/*manifest should be initialized in zrt main*/
static struct UserManifest* s_manifest;
/*runtime information related to channels*/
static struct ZrtChannelRt **s_zrt_channels;
/*directories list based on channels list; for getdents syscall/readdir*/
struct manifest_loaded_directories_t s_manifest_dirs;
/*getdents temp data for subsequent syscalls*/
static struct ReadDirTemp s_readdir_temp = {{-1,0,0,0},-1,-1};
static int s_donotlog=0;

#ifdef DEBUG
static int s_zrt_log_fd = -1;
static char s_logbuf[0x1000];
int debug_handle_get_buf(char **buf){
    if ( s_donotlog != 0 ) return -1; /*switch off log for some functions*/
    *buf = s_logbuf;
    return s_zrt_log_fd;
}
#endif

const struct ZVMChannel *zvm_channels_c( int *channels_count ){
    *channels_count = s_manifest->channels_count;
    return s_manifest->channels;
}

void set_zrt_errno( int zvm_errno )
{
    switch ( zvm_errno ){
    case INVALID_DESC:
        errno = EBADF;
        break;
    case INVALID_BUFFER:
        errno = EFAULT;
        break;
    case INTERNAL_ERR:
    case INVALID_ERROR:
    case OUT_OF_LIMITS:
    case INVALID_MODE:
    case INSANE_SIZE:
    case INSANE_OFFSET:
    case OUT_OF_BOUNDS:
    default:
        errno = zvm_errno;
        break;
    }

    zrt_log( "errno=%d", errno );
}

/*return 0 if specified mode is matches to chan AccessType*/
static int check_channel_access_mode(const struct ZVMChannel *chan, int access_mode)
{
    assert(chan);

    /*check read / write accasebility*/
    int canberead = chan->limits[GetsLimit] && chan->limits[GetSizeLimit];
    int canbewrite = chan->limits[PutsLimit] && chan->limits[PutSizeLimit];

    zrt_log("access_mode=%u, canberead=%d, canbewrite=%d", access_mode, canberead, canbewrite );

    /*reset permissions bits, that are not used currently*/
    access_mode = access_mode & O_ACCMODE;
    switch( access_mode ){
    case O_RDONLY:
        zrt_log("access_mode=%s", "O_RDONLY");
        return canberead>0 ? 0: -1;
    case O_WRONLY:
        zrt_log("access_mode=%s", "O_WRONLY");
        return canbewrite >0 ? 0 : -1;
    case O_RDWR:
    default:
        /*if case1: O_RDWR ; case2: O_RDWR|O_WRONLY logic error handle as O_RDWR*/
        zrt_log("access_mode=%s", "O_RDWR");
        return canberead>0 && canbewrite>0 ? 0 : -1;
    }
    return 1;
}

void debug_mes_zrt_channel_runtime( int handle ){
    const struct ZrtChannelRt *zrt_chan_runtime = s_zrt_channels[handle];
    zrt_log("handle=%d", handle);
    if (zrt_chan_runtime){
        zrt_log("open_mode=%u, flags=%u, sequential_access_pos=%llu, random_access_pos=%llu",
                zrt_chan_runtime->open_mode, zrt_chan_runtime->flags,
                zrt_chan_runtime->sequential_access_pos, zrt_chan_runtime->random_access_pos );
    }
};

void debug_mes_open_flags( int flags )
{
    int all_flags[] = {O_CREAT, O_EXCL, O_TRUNC, O_DIRECT, O_DIRECTORY,
            O_NOATIME, O_APPEND, O_ASYNC, O_SYNC, O_NONBLOCK, O_NDELAY, O_NOCTTY};
    char *all_texts[] = {"O_CREAT", "O_EXCL", "O_TRUNC", "O_DIRECT", "O_DIRECTORY",
            "O_NOATIME", "O_APPEND", "O_ASYNC", "O_SYNC", "O_NONBLOCK", "O_NDELAY", "O_NOCTTY"};
    int i;
    assert( sizeof(all_flags)/sizeof(int) == sizeof(all_texts)/sizeof(char*) );

    for ( i=0; i < sizeof(all_flags)/sizeof(int); i++ ){
        if ( CHECK_FLAG(flags, all_flags[i]) ){
            zrt_log("flag %s=%d", all_texts[i], CHECK_FLAG(flags, all_flags[i]) );
        }
    }
    /*
     * O_CREAT,      cdr =
     * O_EXCL,       cdr
     * O_TRUNC      provided by trusted side
     * O_LARGEFILE,  native support should be
     * O_DIRECT              required _GNU_SOURCE define do we need it?
     * O_DIRECTORY,  cdr     required _GNU_SOURCE define
     * O_NOATIME, ignore     required _GNU_SOURCE define
     * O_NOFOLLOW, ignore    (absent in nacl headers)
     * O_APPEND support for cdr
     * O_CREAT  provided by trusted side
     * O_ASYNC
     * O_CLOEXEC ignore non portable
     * O_SYNC, ignore
     * O_NONBLOCK, O_NDELAY, O_NOCTTY,   does not supported
     * */
}


/* Add channel to runtime data array
 * @return handle on success or if already opened, otherwise -1 and set errno*/
static int open_channel( const char *name, int mode, int flags  )
{
    zrt_log("name=%s, mode=%u, flags=%u", name, mode, flags);
    const struct ZVMChannel *chan = NULL;

    /* search for name through the channels list*/
    int handle;
    for(handle = 0; handle < s_manifest->channels_count; ++handle)
        if(strcmp(s_manifest->channels[handle].name, name) == 0){
            chan = &s_manifest->channels[handle];
            break; /*handle according to matched filename*/
        }

    /* if channel name not matched return error*/
    if ( !chan ){
        set_zrt_errno( ENOENT );
        return -1;
    }
    zrt_log("channel type=%d", s_manifest->channels[handle].type );

    /*return handle if file already opened*/
    if ( s_zrt_channels[handle] && s_zrt_channels[handle]->open_mode >= 0 ){
        zrt_log("channel already opened, handle=%d ", handle );
        return handle;
    }


    /*check access mode for opening channel, limits not checked*/
    if( check_channel_access_mode( chan, mode ) != 0 ){
        zrt_log("can't open channel, handle=%d ", handle );
        set_zrt_errno( EACCES );
        return -1;
    }

    /**DEBUG*******************************************/
    debug_mes_open_flags(flags);
    /**DEBUG*******************************************/

    /*alloc zrt runtime channel object or update existing*/
    if ( !s_zrt_channels[handle] ){
        s_zrt_channels[handle] = calloc( 1, sizeof(struct ZrtChannelRt) );
    }
    s_zrt_channels[handle]->open_mode = mode;
    s_zrt_channels[handle]->flags = flags;

#ifdef DEBUG
    debug_mes_zrt_channel_runtime(handle);
#endif
    return handle;
}


int64_t channel_pos_sequen_get_sequen_put( struct ZrtChannelRt *zrt_channel,
        int8_t whence, int8_t access, int64_t offset ){
    /*seek get supported by channel for any modes*/
    if ( EPosGet == whence ) return zrt_channel->sequential_access_pos;
    switch ( access ){
    case EPosSeek:
        /*seek set does not supported for this channel type*/
        set_zrt_errno( ESPIPE );
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


int64_t channel_pos_random_get_sequen_put( struct ZrtChannelRt *zrt_channel,
        int8_t whence, int8_t access, int64_t offset ){
    switch ( access ){
    case EPosSeek:
        if ( CHECK_FLAG( zrt_channel->open_mode, O_RDONLY )
                || CHECK_FLAG( zrt_channel->open_mode, O_RDWR ) )
        {
            if ( EPosGet == whence ) return zrt_channel->random_access_pos;
            SET_SAFE_OFFSET( whence, &zrt_channel->random_access_pos, offset );
        }
        else if ( CHECK_FLAG( zrt_channel->open_mode, O_WRONLY ) == 1 )
        {
            if ( EPosGet == whence ) return zrt_channel->sequential_access_pos;
            else{
                /*it's does not supported for seeking sequential write*/
                set_zrt_errno( ESPIPE );
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

int64_t channel_pos_sequen_get_random_put(struct ZrtChannelRt *zrt_channel,
        int8_t whence, int8_t access, int64_t offset){
    switch ( access ){
    case EPosSeek:
        if ( CHECK_FLAG( zrt_channel->open_mode, O_WRONLY ) == 1
                || CHECK_FLAG( zrt_channel->open_mode, O_RDWR ) == 1 )
        {
            if ( EPosGet == whence ) return zrt_channel->random_access_pos;
            SET_SAFE_OFFSET( whence, &zrt_channel->random_access_pos, offset );
        }
        else if ( CHECK_FLAG( zrt_channel->open_mode, O_RDONLY )  == 1)
        {
            if ( EPosGet == whence ) return zrt_channel->sequential_access_pos;
            else{
                /*it's does not supported for seeking sequential read*/
                set_zrt_errno( ESPIPE );
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

int64_t channel_pos_random_get_random_put(struct ZrtChannelRt *zrt_channel,
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

int64_t channel_pos( int handle, int8_t whence, int8_t access, int64_t offset ){
    if ( handle>=0 && handle < s_manifest->channels_count && s_zrt_channels[handle] ){
        struct ZrtChannelRt *zrt_channel = s_zrt_channels[handle];
        assert( zrt_channel );
        int8_t access_type = s_manifest->channels[handle].type;
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
        set_zrt_errno( EBADF );
    }
    return -1;
}



/*
 * ZRT IMPLEMENTATION OF NACL SYSCALLS
 * each nacl syscall must be implemented or, at least, mocked. no exclusions!
 */

SYSCALL_MOCK(nan, 0)
SYSCALL_MOCK(null, 0)
SYSCALL_MOCK(nameservice, 0)
SYSCALL_MOCK(dup, -EPERM) /* duplicate the given file handle. n/a in the simple zrt version */
SYSCALL_MOCK(dup2, -EPERM) /* duplicate the given file handle. n/a in the simple zrt version */


/*
 * return channel handle by given name. if given flags
 * doesn't answer channel's type/limits
 */
static int32_t zrt_open(uint32_t *args)
{
    SHOWID;
    errno=0;
    char* name = (char*)args[0];
    int flags = (int)args[1];
    int mode = (int)args[2];
    zrt_log("name=%s", name);
    debug_mes_open_flags(flags);

    if ( CHECK_FLAG(flags, O_DIRECTORY) == 0 ){
        return open_channel( name, flags, mode );
    }
    else { /*trying to open directory*/
        struct dir_data_t *dir = match_dir_in_directory_list( &s_manifest_dirs, name, strlen(name));
        /*if valid directory path matched */
        if ( dir != NULL ){
            if ( dir->open_mode < 0 ){ /*if not opened*/
                /*if trying open in read only*/
                if ( CHECK_FLAG(mode, O_RDONLY) ){  /*it's allowed*/
                    dir->open_mode = mode;
                    return dir->handle;
                }
                else{  /*Not allowed read-write / write access*/
                    set_zrt_errno( EACCES );
                    return -1;
                }
            }
            else { /*already opened, just return handle*/
                return dir->handle;
            }
        }
        else{
            /*no matched directory*/
            set_zrt_errno( ENOENT );
            return -1;
        }
    }
    return 0;
}


/* do nothing but checks given handle */
static int32_t zrt_close(uint32_t *args)
{
    SHOWID;
    errno = 0;
    int handle = (int)args[0];
    zrt_log("handle=%d", handle);

    /*if valid handle and file was opened previously then perform file close*/
    if ( handle < s_manifest->channels_count &&
            s_zrt_channels[handle] && s_zrt_channels[handle]->open_mode >=0  )
    {
        s_zrt_channels[handle]->random_access_pos = s_zrt_channels[handle]->sequential_access_pos = 0;
        s_zrt_channels[handle]->maxsize = 0;
        s_zrt_channels[handle]->open_mode = -1;
        return 0;
    }
    else{ /*search handle in directories list*/
        int i;
        for ( i=0; i < s_manifest_dirs.dircount; i++ ){
            /*if matched handle*/
            if ( s_manifest_dirs.dir_array[i].handle == handle &&
                    s_manifest_dirs.dir_array[i].open_mode >= 0 )
            {
                /*close opened dir*/
                s_manifest_dirs.dir_array[i].open_mode = -1;
                return 0;
            }
        }
        /*no matched open dir handle*/
        set_zrt_errno( EBADF );
        return -1;
    }

    return 0;
}


/* read the file with the given handle number */
static int32_t zrt_read(uint32_t *args)
{
    SHOWID;
    errno = 0;
    int handle = (int)args[0];
    void *buf = (void*)args[1];
    int64_t length = (int64_t)args[2];
    int32_t readed = 0;

    /*file not opened, bad descriptor*/
    if ( handle < 0 || handle >= s_manifest->channels_count ||
            !s_zrt_channels[handle] ||
            s_zrt_channels[handle]->open_mode < 0 )
    {
        zrt_log("invalid file descriptor handle=%d", handle);
        set_zrt_errno( EBADF );
        return -1;
    }
    zrt_log( "channel name=%s", s_manifest->channels[handle].name );
    debug_mes_zrt_channel_runtime( handle );

    /*check if file was not opened for reading*/
    int mode= O_ACCMODE & s_zrt_channels[handle]->open_mode;
    if ( mode != O_RDONLY && mode != O_RDWR  )
    {
        zrt_log("file open_mode=%u not allowed read", mode);
        set_zrt_errno( EINVAL );
        return -1;
    }

    int64_t pos = channel_pos(handle, EPosGet, EPosRead, 0);
    if ( CHECK_NEW_POS( pos+length ) != 0 ){
        zrt_log("file bad pos=%lld", pos);
        set_zrt_errno( EOVERFLOW );
        return -1;
    }

    readed = zvm_pread(handle, buf, length, pos );
    if(readed > 0) channel_pos(handle, EPosSetRelative, EPosRead, readed);

    zrt_log("channel handle=%d, bytes readed=%d", handle, readed);

    if ( readed < 0 ){
        set_zrt_errno( zvm_errno() );
        return -1;
    }
    ///ZEROVM Obsolete design will removed soon
    return readed;
}

/* example how to implement zrt syscall */
static int32_t zrt_write(uint32_t *args)
{
    SHOWID;
    int handle = (int)args[0];
    void *buf = (void*)args[1];
    int64_t length = (int64_t)args[2];
    int32_t wrote = 0;

    if ( handle < 3 ) s_donotlog = 1;
    else             s_donotlog = 0;

    /*file not opened, bad descriptor*/
    if ( handle < 0 || handle >= s_manifest->channels_count ||
            !s_zrt_channels[handle] ||
            s_zrt_channels[handle]->open_mode < 0  )
    {
        zrt_log("invalid file descriptor handle=%d", handle);
        set_zrt_errno( EBADF );
        return -1;
    }

    zrt_log( "channel name=%s", s_manifest->channels[handle].name );
    debug_mes_zrt_channel_runtime( handle );

    /*check if file was not opened for writing*/
    int mode= O_ACCMODE & s_zrt_channels[handle]->open_mode;
    if ( mode != O_WRONLY && mode != O_RDWR  )
    {
        zrt_log("file open_mode=%u not allowed write", mode);
        set_zrt_errno( EINVAL );
        return -1;
    }

    int64_t pos = channel_pos(handle, EPosGet, EPosWrite, 0);
    if ( CHECK_NEW_POS( pos+length ) != 0 ){
        set_zrt_errno( EOVERFLOW );
        return -1;
    }

    wrote = zvm_pwrite(handle, buf, length, pos );
    if(wrote > 0) channel_pos(handle, EPosSetRelative, EPosWrite, wrote);
    zrt_log("channel handle=%d, bytes wrote=%d", handle, wrote);

    if ( wrote < 0 ){
        set_zrt_errno( zvm_errno() );
        return -1;
    }

    s_donotlog = 0;
    return wrote;
}

/*
 * seek for the new zerovm channels design
 */
static int32_t zrt_lseek(uint32_t *args)
{
    SHOWID;
    errno = 0;
    int32_t handle = (int32_t)args[0];
    off_t offset = *((off_t*)args[1]);
    int whence = (int)args[2];

    zrt_log("handle=%d, offset=%d", handle, (int)offset);
    debug_mes_zrt_channel_runtime( handle );

    /* check handle */
    if(handle < 0 || handle >= s_manifest->channels_count){
        set_zrt_errno( EBADF );
        return -1;
    }
    zrt_log( "channel name=%s", s_manifest->channels[handle].name );

    switch(whence)
    {
    case SEEK_SET:
        zrt_log("whence=%s", "SEEK_SET");
        offset = channel_pos(handle, EPosSetAbsolute, EPosSeek, offset );
        break;
    case SEEK_CUR:
        zrt_log("whence=%s", "SEEK_CUR");
        if ( !offset )
            offset = channel_pos(handle, EPosGet, EPosSeek, offset );
        else
            offset = channel_pos(handle, EPosSetRelative, EPosSeek, offset );
        break;
    case SEEK_END:
        zrt_log("whence=%s, maxsize=%lld", "SEEK_END", s_zrt_channels[handle]->maxsize);
        /*use runtime size instead static size in zvm channel*/
        offset = channel_pos(handle, EPosSetAbsolute, EPosSeek, s_zrt_channels[handle]->maxsize + offset );
        break;
    default:
        set_zrt_errno( EPERM ); /* in advanced version should be set to conventional value */
        return -1;
    }

    /*
     * return current position in a special way since 64 bits
     * doesn't fit to return code (32 bits)
     */
    *(off_t *)args[1] = offset;
    return offset;
}


SYSCALL_MOCK(ioctl, -EINVAL) /* not implemented in the simple version of zrtlib */


void debug_mes_stat(struct nacl_abi_stat *stat){
    zrt_log("st_dev=%lld", stat->nacl_abi_st_dev);
    zrt_log("st_ino=%lld", stat->nacl_abi_st_ino);
    zrt_log("nlink=%d", stat->nacl_abi_st_nlink);
    zrt_log("st_mode=%d", stat->nacl_abi_st_mode);
    zrt_log("st_blksize=%d", stat->nacl_abi_st_blksize);
    zrt_log("st_size=%lld", stat->nacl_abi_st_size);
    zrt_log("st_blocks=%d", stat->nacl_abi_st_blocks);
}

/*used by stat, fstat; set stat based on channel type*/
static void set_stat(struct nacl_abi_stat *stat, int handle)
{
    zrt_log("handle=%d", handle);

    int nlink = 1;
    uint32_t permissions = 0;
    int64_t size=4096;
    uint32_t blksize =1;
    uint64_t ino;

    if ( handle >= s_manifest->channels_count ){ /*if handle is dir handle*/
        struct dir_data_t *d = match_handle_in_directory_list( &s_manifest_dirs, handle );
        nlink = d->nlink;
        ino = INODE_FROM_HANDLE(d->handle);
        permissions |= S_IRUSR | S_IFDIR;
    }
    else{ /*channel handle*/
        if ( CHECK_FLAG( s_zrt_channels[handle]->open_mode, O_RDONLY) ){
            permissions |= S_IRUSR;
        }
        else if ( CHECK_FLAG( s_zrt_channels[handle]->open_mode, O_WRONLY) ){
            permissions |= S_IWUSR;
        }
        else{
            permissions |= S_IRUSR | S_IWUSR | S_IFBLK; //only cdr has w/r access, it's block device
            blksize = 4096;
        }
        permissions |= S_IFREG;
        ino = INODE_FROM_HANDLE( handle );

        size = MAX( s_zrt_channels[handle]->maxsize, s_manifest->channels[handle].size ); /* size in bytes */
    }

    /* return stat object */
    stat->nacl_abi_st_dev = 2049;     /* ID of device containing handle */
    stat->nacl_abi_st_ino = ino;     /* inode number */
    stat->nacl_abi_st_nlink = nlink;      /* number of hard links */;
    stat->nacl_abi_st_uid = 1000;     /* user ID of owner */
    stat->nacl_abi_st_gid = 1000;     /* group ID of owner */
    stat->nacl_abi_st_rdev = 0;       /* device ID (if special handle) */
    stat->nacl_abi_st_mode = permissions;
    stat->nacl_abi_st_blksize = blksize;        /* block size for file system I/O */
    stat->nacl_abi_st_size = size;
    stat->nacl_abi_st_blocks =               /* number of 512B blocks allocated */
            ((stat->nacl_abi_st_size + stat->nacl_abi_st_blksize - 1)
                    / stat->nacl_abi_st_blksize) * stat->nacl_abi_st_blksize / 512;

    /* files are not allowed to have real date/time */
    stat->nacl_abi_st_atime = 0;      /* time of the last access */
    stat->nacl_abi_st_mtime = 0;      /* time of the last modification */
    stat->nacl_abi_st_ctime = 0;      /* time of the last status change */
    stat->nacl_abi_st_atimensec = 0;
    stat->nacl_abi_st_mtimensec = 0;
    stat->nacl_abi_st_ctimensec = 0;

    debug_mes_stat(stat);

}
/*
 * return synthetic channel information
 * todo(d'b): the function needs update after the channels design will complete
 */
static int32_t zrt_stat(uint32_t *args)
{
    SHOWID;
    errno = 0;
    const char *file = (const char*)args[0];
    struct nacl_abi_stat *sbuf = (struct nacl_abi_stat *)args[1];
    int handle;

    struct ZVMChannel *channel = NULL;
    struct dir_data_t *dir = NULL;

    zrt_log("file=%s", file);

    if(file == NULL){
        set_zrt_errno( EFAULT );
        return -1;
    }

    for(handle = 0; handle < s_manifest->channels_count; ++handle)
        if(!strcmp(file, s_manifest->channels[handle].name))
            channel = &s_manifest->channels[handle];

    if ( channel ){
        set_stat( sbuf, handle);
        return 0;
    }
    else if ( (dir=match_dir_in_directory_list(&s_manifest_dirs, file, strlen(file))) ){
        set_stat( sbuf, dir->handle);
        return 0;
    }
    else{
        set_zrt_errno( ENOENT );
        return -1;
    }

    return 0;
}


/* return synthetic channel information */
static int32_t zrt_fstat(uint32_t *args)
{
    SHOWID;
    errno = 0;
    int handle = (int)args[0];
    struct nacl_abi_stat *sbuf = (struct nacl_abi_stat *)args[1];

    zrt_log("handle=%d", handle);

    /* check if user request contain the proper file handle */
    if(handle >= 0 && handle < s_manifest->channels_count){
        set_stat( sbuf, handle); /*channel handle*/
    }
    else if ( match_handle_in_directory_list(&s_manifest_dirs, handle) ){
        set_stat( sbuf, handle); /*dir handle*/
    }
    else{
        set_zrt_errno( EBADF );
        return -1;
    }

    return 0;
}

SYSCALL_MOCK(chmod, -EPERM) /* in a simple version of zrt chmod is not allowed */

/*
 * following 3 functions (sysbrk, mmap, munmap) is the part of the
 * new memory engine. the new allocate specified amount of ram before
 * user code start and then user can only obtain that allocated memory.
 * zrt lib will help user to do it transparently.
 */

/* change space allocation */
static int32_t zrt_sysbrk(uint32_t *args)
{
    SHOWID;
    int32_t retcode;

    zvm_syscallback(0); /* uninstall syscallback */
    retcode = NaCl_sysbrk(args[0]); /* invoke syscall directly */
    zvm_syscallback((intptr_t)syscall_director); /* reinstall syscallback */

    return retcode;
}

/* map region of memory */
static int32_t zrt_mmap(uint32_t *args)
{
    SHOWID;
    int32_t retcode;

    zvm_syscallback(0); /* uninstall syscallback */
    retcode = NaCl_mmap(args[0], args[1], args[2], args[3], args[4], args[5]);
    zvm_syscallback((intptr_t)syscall_director); /* reinstall syscallback */

    return retcode;
}

/*
 * unmap region of memory
 * note: zerovm doesn't use it in memory management.
 * instead of munmap it use mmap with protection 0
 */
static int32_t zrt_munmap(uint32_t *args)
{
    SHOWID;
    int32_t retcode;

    zvm_syscallback(0); /* uninstall syscallback */
    retcode = NaCl_munmap(args[0], args[1]);
    zvm_syscallback((intptr_t)syscall_director); /* reinstall syscallback */

    return retcode;
}


static int32_t zrt_getdents(uint32_t *args)
{
    SHOWID;
    errno=0;
    int handle = (int)args[0];
    char *buf = (char*)args[1];
    uint32_t count = args[2];
    zrt_log( "handle=%d, count=%d", handle, count );

    /* check null and  make sure the buffer is aligned */
    if ( !buf || 0 != ((sizeof(unsigned long) - 1) & (uintptr_t) buf)) {
        set_zrt_errno( EINVAL );
        return -1;
    }

    int retval = readdir_to_buffer( handle, buf, count, &s_readdir_temp, s_manifest, &s_manifest_dirs );

    zrt_log( "retval=%d", retval );
    return retval;
}


/*
 * exit. most important syscall. without it the user program
 * cannot terminate correctly.
 */
static int32_t zrt_exit(uint32_t *args)
{
    /* no need to check args for NULL. it is always set by syscall_manager */
    SHOWID;
    zvm_exit(args[0]);
    return 0; /* unreachable */
}

SYSCALL_MOCK(getpid, 0)
SYSCALL_MOCK(sched_yield, 0)
SYSCALL_MOCK(sysconf, 0)

/* if given in manifest let user to have it */
#define TIMESTAMP_STR "TimeStamp="
#define TIMESTAMP_STRLEN strlen("TimeStamp=")
static int32_t zrt_gettimeofday(uint32_t *args)
{
    SHOWID;
    struct nacl_abi_timeval  *tv = (struct nacl_abi_timeval *)args[0];
    char *stamp = NULL;
    int i;

    /* get time stamp from the environment */
    for(i = 0; s_manifest->envp[i] != NULL; ++i)
    {
        if(strncmp(s_manifest->envp[i], TIMESTAMP_STR, TIMESTAMP_STRLEN) != 0)
            continue;

        stamp = s_manifest->envp[i] + TIMESTAMP_STRLEN;
        break;
    }

    /* check if timestampr is set */
    if(stamp == NULL || !*stamp) return -EPERM;

    /* check given arguments validity */
    if(!tv) return -EFAULT;

    tv->nacl_abi_tv_usec = 0; /* to simplify code. yet we can't get msec from nacl code */
    tv->nacl_abi_tv_sec = atoi(stamp); /* manifest always contain decimal values */

    return 0;
}

/*
 * should never be implemented if we want deterministic behaviour
 * note: but we can allow to return each time synthetic value
 * warning! after checking i found that nacl is not using it, so this
 *          function is useless for current nacl sdk version.
 */
SYSCALL_MOCK(clock, -EPERM)
SYSCALL_MOCK(nanosleep, 0)
SYSCALL_MOCK(imc_makeboundsock, 0)
SYSCALL_MOCK(imc_accept, 0)
SYSCALL_MOCK(imc_connect, 0)
SYSCALL_MOCK(imc_sendmsg, 0)
SYSCALL_MOCK(imc_recvmsg, 0)
SYSCALL_MOCK(imc_mem_obj_create, 0)
SYSCALL_MOCK(imc_socketpair, 0)
SYSCALL_MOCK(mutex_create, 0)
SYSCALL_MOCK(mutex_lock, 0)
SYSCALL_MOCK(mutex_trylock, 0)
SYSCALL_MOCK(mutex_unlock, 0)
SYSCALL_MOCK(cond_create, 0)
SYSCALL_MOCK(cond_wait, 0)
SYSCALL_MOCK(cond_signal, 0)
SYSCALL_MOCK(cond_broadcast, 0)
SYSCALL_MOCK(cond_timed_wait_abs, 0)
SYSCALL_MOCK(thread_create, 0)
SYSCALL_MOCK(thread_exit, 0)
SYSCALL_MOCK(tls_init, 0)
SYSCALL_MOCK(thread_nice, 0)

/*
 * get tls from zerovm. can be replaced with untrusted version
 * after "blob library" loader will be ready
 */
static int32_t zrt_tls_get(uint32_t *args)
{
    /* switch off spam
     * SHOWID;*/
    int32_t retcode;

    zvm_syscallback(0); /* uninstall syscallback */
    retcode = NaCl_tls_get(); /* invoke syscall directly */
    zvm_syscallback((intptr_t)syscall_director); /* reinstall syscallback */

    return retcode;
}

SYSCALL_MOCK(second_tls_set, 0)

/*
 * get second tls.
 * since we only have single thread use instead of 2nd tls 1st one
 */
static int32_t zrt_second_tls_get(uint32_t *args)
{
    SHOWID;
    return zrt_tls_get(NULL);
}

SYSCALL_MOCK(sem_create, 0)
SYSCALL_MOCK(sem_wait, 0)
SYSCALL_MOCK(sem_post, 0)
SYSCALL_MOCK(sem_get_value, 0)
SYSCALL_MOCK(dyncode_create, 0)
SYSCALL_MOCK(dyncode_modify, 0)
SYSCALL_MOCK(dyncode_delete, 0)
SYSCALL_MOCK(test_infoleak, 0)

/*
 * array of the pointers to zrt syscalls
 *
 * each zrt function (syscall) has uniform prototype: int32_t syscall(uint32_t *args)
 * where "args" pointer to syscalls' arguments. number and types of arguments
 * can be found in the nacl "nacl_syscall_handlers.c" file.
 */
int32_t (*zrt_syscalls[])(uint32_t*) = {
        zrt_nan,                   /* 0 -- not implemented syscall */
        zrt_null,                  /* 1 -- empty syscall. does nothing */
        zrt_nameservice,           /* 2 */
        zrt_nan,                   /* 3 -- not implemented syscall */
        zrt_nan,                   /* 4 -- not implemented syscall */
        zrt_nan,                   /* 5 -- not implemented syscall */
        zrt_nan,                   /* 6 -- not implemented syscall */
        zrt_nan,                   /* 7 -- not implemented syscall */
        zrt_dup,                   /* 8 */
        zrt_dup2,                  /* 9 */
        zrt_open,                  /* 10 */
        zrt_close,                 /* 11 */
        zrt_read,                  /* 12 */
        zrt_write,                 /* 13 */
        zrt_lseek,                 /* 14 */
        zrt_ioctl,                 /* 15 */
        zrt_stat,                  /* 16 */
        zrt_fstat,                 /* 17 */
        zrt_chmod,                 /* 18 */
        zrt_nan,                   /* 19 -- not implemented syscall */
        zrt_sysbrk,                /* 20 */
        zrt_mmap,                  /* 21 */
        zrt_munmap,                /* 22 */
        zrt_getdents,              /* 23 */
        zrt_nan,                   /* 24 -- not implemented syscall */
        zrt_nan,                   /* 25 -- not implemented syscall */
        zrt_nan,                   /* 26 -- not implemented syscall */
        zrt_nan,                   /* 27 -- not implemented syscall */
        zrt_nan,                   /* 28 -- not implemented syscall */
        zrt_nan,                   /* 29 -- not implemented syscall */
        zrt_exit,                  /* 30 -- must use trap:exit() */
        zrt_getpid,                /* 31 */
        zrt_sched_yield,           /* 32 */
        zrt_sysconf,               /* 33 */
        zrt_nan,                   /* 34 -- not implemented syscall */
        zrt_nan,                   /* 35 -- not implemented syscall */
        zrt_nan,                   /* 36 -- not implemented syscall */
        zrt_nan,                   /* 37 -- not implemented syscall */
        zrt_nan,                   /* 38 -- not implemented syscall */
        zrt_nan,                   /* 39 -- not implemented syscall */
        zrt_gettimeofday,          /* 40 */
        zrt_clock,                 /* 41 */
        zrt_nanosleep,             /* 42 */
        zrt_nan,                   /* 43 -- not implemented syscall */
        zrt_nan,                   /* 44 -- not implemented syscall */
        zrt_nan,                   /* 45 -- not implemented syscall */
        zrt_nan,                   /* 46 -- not implemented syscall */
        zrt_nan,                   /* 47 -- not implemented syscall */
        zrt_nan,                   /* 48 -- not implemented syscall */
        zrt_nan,                   /* 49 -- not implemented syscall */
        zrt_nan,                   /* 50 -- not implemented syscall */
        zrt_nan,                   /* 51 -- not implemented syscall */
        zrt_nan,                   /* 52 -- not implemented syscall */
        zrt_nan,                   /* 53 -- not implemented syscall */
        zrt_nan,                   /* 54 -- not implemented syscall */
        zrt_nan,                   /* 55 -- not implemented syscall */
        zrt_nan,                   /* 56 -- not implemented syscall */
        zrt_nan,                   /* 57 -- not implemented syscall */
        zrt_nan,                   /* 58 -- not implemented syscall */
        zrt_nan,                   /* 59 -- not implemented syscall */
        zrt_imc_makeboundsock,     /* 60 */
        zrt_imc_accept,            /* 61 */
        zrt_imc_connect,           /* 62 */
        zrt_imc_sendmsg,           /* 63 */
        zrt_imc_recvmsg,           /* 64 */
        zrt_imc_mem_obj_create,    /* 65 */
        zrt_imc_socketpair,        /* 66 */
        zrt_nan,                   /* 67 -- not implemented syscall */
        zrt_nan,                   /* 68 -- not implemented syscall */
        zrt_nan,                   /* 69 -- not implemented syscall */
        zrt_mutex_create,          /* 70 */
        zrt_mutex_lock,            /* 71 */
        zrt_mutex_trylock,         /* 72 */
        zrt_mutex_unlock,          /* 73 */
        zrt_cond_create,           /* 74 */
        zrt_cond_wait,             /* 75 */
        zrt_cond_signal,           /* 76 */
        zrt_cond_broadcast,        /* 77 */
        zrt_nan,                   /* 78 -- not implemented syscall */
        zrt_cond_timed_wait_abs,   /* 79 */
        zrt_thread_create,         /* 80 */
        zrt_thread_exit,           /* 81 */
        zrt_tls_init,              /* 82 */
        zrt_thread_nice,           /* 83 */
        zrt_tls_get,               /* 84 */
        zrt_second_tls_set,        /* 85 */
        zrt_second_tls_get,        /* 86 */
        zrt_nan,                   /* 87 -- not implemented syscall */
        zrt_nan,                   /* 88 -- not implemented syscall */
        zrt_nan,                   /* 89 -- not implemented syscall */
        zrt_nan,                   /* 90 -- not implemented syscall */
        zrt_nan,                   /* 91 -- not implemented syscall */
        zrt_nan,                   /* 92 -- not implemented syscall */
        zrt_nan,                   /* 93 -- not implemented syscall */
        zrt_nan,                   /* 94 -- not implemented syscall */
        zrt_nan,                   /* 95 -- not implemented syscall */
        zrt_nan,                   /* 96 -- not implemented syscall */
        zrt_nan,                   /* 97 -- not implemented syscall */
        zrt_nan,                   /* 98 -- not implemented syscall */
        zrt_nan,                   /* 99 -- not implemented syscall */
        zrt_sem_create,            /* 100 */
        zrt_sem_wait,              /* 101 */
        zrt_sem_post,              /* 102 */
        zrt_sem_get_value,         /* 103 */
        zrt_dyncode_create,        /* 104 */
        zrt_dyncode_modify,        /* 105 */
        zrt_dyncode_delete,        /* 106 */
        zrt_nan,                   /* 107 -- not implemented syscall */
        zrt_nan,                   /* 108 -- not implemented syscall */
        zrt_test_infoleak          /* 109 */
};


void zrt_setup( struct UserManifest* manifest ){
    int i;
    s_manifest = manifest;

    /*array of pointers*/
    s_zrt_channels = calloc( manifest->channels_count, sizeof(struct ZrtChannelRt*) );
    s_zrt_log_fd = open_channel( ZRT_LOG_NAME, O_WRONLY, 0 ); /*open log channel*/

    //zrt_log( lformat(s_logbuf, "%s", "started") );
    zrt_log_str( "started" );

    s_manifest_dirs.dircount=0;
    process_channels_create_dir_list( manifest->channels, manifest->channels_count, &s_manifest_dirs );

#ifdef DEBUG
    zrt_log("dirs count loaded from manifest=%d", s_manifest_dirs.dircount);
    for ( i=0; i < s_manifest_dirs.dircount; i++ ){
        zrt_log( "dir[%d].handle=%d; .path=%s; .nlink=%d", i,
                s_manifest_dirs.dir_array[i].handle,
                s_manifest_dirs.dir_array[i].path,
                s_manifest_dirs.dir_array[i].nlink);
    }
#endif
    /*preallocate sdtin, stdout, stderr channels*/
    open_channel( DEV_STDIN, O_RDONLY, 0 );
    open_channel( DEV_STDOUT, O_WRONLY, 0 );
    open_channel( DEV_STDERR, O_WRONLY, 0 );
}


