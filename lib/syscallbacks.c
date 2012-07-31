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
#include <assert.h>

#include "zvm.h"
#include "zrt.h"
#include "syscallbacks.h"


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
#define ZRT_LOG_NAME "/dev/zrtlog"
#define SHOWID {log_msg((char*)__func__); log_msg("() is called\n");}
#else
#define SHOWID
#endif

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


/*manifest should be initialized in zrt main*/
static struct UserManifest* s_manifest;

/*runtime information related to channels*/
static struct ZrtChannelRt **s_zrt_channels;
static int s_zrt_log_fd = -1;

#ifdef DEBUG
void log_msg(const char *msg)
{
    if ( s_zrt_log_fd < 0 ) return;
    int length = zvm_pwrite(s_zrt_log_fd, msg, strlen(msg), channel_pos(s_zrt_log_fd, EPosGet, 0) );
    if(length > 0) channel_pos(s_zrt_log_fd, EPosSetRelative, length);
}

size_t strlen(const char *string)
{
    const char *s;
    s = string;
    while (*s)
        s++;
    return s - string;
}

char *strrev(char *str)
{
    char *p1, *p2;
    if (!str || !*str)
        return str;
    for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
    {
        *p1 ^= *p2;
        *p2 ^= *p1;
        *p1 ^= *p2;
    }
    return str;
}

char *itoa(int n, char *s, int b) {
    static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    int i=0, sign;
    if ((sign = n) < 0)
        n = -n;
    do {
        s[i++] = digits[n % b];
    } while ((n /= b) > 0);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    return strrev(s);
}

int log_int(int dec)
{
    char log_msg_text[30];
    itoa(dec, (char*)log_msg_text, 10);
    log_msg(log_msg_text);
    return 0;
}
#endif

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

    /**DEBUG*******************************************/
    log_msg( "set errno= " ); log_int( errno ); log_msg( "\n" );
    /**DEBUG*******************************************/
}

/*return 0 if specified mode is matches to chan AccessType*/
static int check_channel_access_mode(const struct ZVMChannel *chan, int access_mode)
{
    assert(chan);

    /*check read / write accasebility*/
    int canberead = chan->limits[GetsLimit] && chan->limits[GetSizeLimit];
    int canbewrite = chan->limits[PutsLimit] && chan->limits[PutSizeLimit];

    log_msg( "canberead = " ); log_int( canberead );
    log_msg( " canbewrite = " ); log_int( canbewrite ); log_msg( "\n" );

    /*reset permissions bits, that are not used currently*/
    access_mode = access_mode & O_ACCMODE;

    switch( access_mode ){
    case O_RDONLY:
        /**DEBUG*******************************************/
        log_msg( " O_RDONLY " );
        /**DEBUG*******************************************/
        return canberead>0 ? 0: -1;
    case O_WRONLY:
        /**DEBUG*******************************************/
        log_msg( " O_WRONLY " );
        /**DEBUG*******************************************/
        return canbewrite >0 ? 0 : -1;
    case O_RDWR:
    default:
        /*if case1: O_RDWR ; case2: O_RDWR|O_WRONLY logic error handle as O_RDWR*/
        /**DEBUG*******************************************/
        log_msg( " O_RDWR " );
        /**DEBUG*******************************************/
        return canberead>0 && canbewrite>0 ? 0 : -1;
    }
    return 1;
}

void debug_mes_zrt_channel_runtime( int handle ){
    const struct ZrtChannelRt *zrt_chan_runtime = s_zrt_channels[handle];
    /**DEBUG*******************************************/
    log_msg( "channel handle=" ); log_int(handle);
    if (zrt_chan_runtime){
        log_msg( " mode=" ); log_int( zrt_chan_runtime->open_mode );
        log_msg( " flags=" ); log_int( zrt_chan_runtime->flags );
        log_msg( " sequential_access_pos=" ); log_int( zrt_chan_runtime->sequential_access_pos );
        log_msg( " random_access_pos=" ); log_int( zrt_chan_runtime->random_access_pos );
    }
    log_msg( "\nlimits[GetsLimit]=" ); log_int( s_manifest->channels[handle].limits[GetsLimit] );
    log_msg( " limits[GetSizeLimit]=" ); log_int( s_manifest->channels[handle].limits[GetSizeLimit] );
    log_msg( "\nlimits[PutsLimit]=" ); log_int( s_manifest->channels[handle].limits[PutsLimit] );
    log_msg( " limits[PutSizeLimit]=" ); log_int( s_manifest->channels[handle].limits[PutSizeLimit] );

    log_msg( "\n" );
    /**DEBUG*******************************************/
};

void debug_mes_open_flags( int flags )
{
    //000110110110
    //    10000000   O_EXCL
    //000100000000   O_NOCTTY
    /**DEBUG*******************************************/
    log_msg( "[O_CREAT=" ); log_int( (flags&O_CREAT) == O_CREAT );
    log_msg( ", O_EXCL=" ); log_int( (flags&O_EXCL) == O_EXCL );//128, 10000000
    log_msg( ", O_TRUNC=" ); log_int( (flags&O_TRUNC) == O_TRUNC );
#ifdef _GNU_SOURCE
    log_msg( "]\n[O_DIRECT=" ); log_int( (flags&O_DIRECT) == O_DIRECT );
    log_msg( ", O_DIRECTORY=" ); log_int( (flags&O_DIRECTORY) == O_DIRECTORY );
    log_msg( ", O_NOATIME=" ); log_int( (flags&O_NOATIME) == O_NOATIME );
#endif
    log_msg( "]\n[O_APPEND=" ); log_int( flags&O_APPEND );
    log_msg( ", O_CREAT=" ); log_int( flags&O_CREAT );
    log_msg( ", O_TRUNC=" ); log_int( flags&O_TRUNC );
    log_msg( "]\n[O_ASYNC=" ); log_int( flags&O_ASYNC );
    log_msg( ", O_SYNC=" ); log_int( flags&O_SYNC );
    log_msg( ", O_NONBLOCK=" ); log_int( flags&O_NONBLOCK );
    log_msg( ", O_NDELAY=" ); log_int( flags&O_NDELAY );
    log_msg( ", O_NOCTTY=" ); log_int( flags&O_NOCTTY ); //256=000100000000
    log_msg( "]\n" );
   /*
    * O_CREAT,      cdr =
    * O_EXCL,       cdr
    * O_TRUNC
    * O_LARGEFILE,  native support should be
    * O_DIRECT              required _GNU_SOURCE define do we need it?
    * O_DIRECTORY,  cdr     required _GNU_SOURCE define
    * O_NOATIME, ignore     required _GNU_SOURCE define
    * O_NOFOLLOW, ignore    (absent in nacl headers)
    * O_APPEND support for cdr
    * O_CREAT  provided by trusted side
    * O_TRUNC  provided by trusted side
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
    log_msg( __func__ ); log_msg( " name=" ); log_msg( name ); log_msg("\n");
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

    /*return handle if file already opened*/
    if ( s_zrt_channels[handle] && s_zrt_channels[handle]->open_mode >= 0 ){
        log_msg( "opening existing handle=" ); log_int(handle); log_msg("\n");
        return handle;
    }

//// ZEROVM BUG WORK AROUND, incorrectly limits parse. temporarily commented code section below
//    /*check access mode for opening channel, limits not checked*/
//    if( check_channel_access_mode( chan, mode ) != 0 ){
//        log_msg( "can't open channel=" ); log_int(handle);
//        log_msg( "[ flags =" ); log_int( flags );
//        log_msg( " mode=" ); log_int( mode ); log_msg("]\n");
//        set_zrt_errno( EACCES );
//        return -1;
//    }
//// ZEROVM BUG WORK AROUND

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


int64_t channel_pos( int handle, int8_t pos_whence, int64_t offset ){
    /**DEBUG*******************************************/
    /*do not log if current pos is calculating for logger, to avoid recursion*/
//    if ( handle != s_zrt_log_fd ){
//        log_msg( __func__ ); log_msg( " handle=" ); log_int( handle );
//        log_msg( " pos_whence=" ); log_int( pos_whence );
//        log_msg( " user offset=" ); log_int( offset );
//    }
    /**DEBUG*******************************************/
    int64_t *pos_p = NULL;
    if ( handle>=0 && handle < s_manifest->channels_count && s_zrt_channels[handle] ){
        int mode = s_zrt_channels[handle]->open_mode;
        switch( s_manifest->channels[handle].type ){
        case SGetSPut:  // sequential_pos - read write
            pos_p = &s_zrt_channels[handle]->sequential_access_pos;
            break;
        case SGetRPut:  // sequential_pos - read, random_pos     - write
            if ( mode && O_RDONLY )
                pos_p = &s_zrt_channels[handle]->sequential_access_pos;
            else
                pos_p = &s_zrt_channels[handle]->random_access_pos;
            break;
        case RGetSPut:  // random_pos     - read, sequential_pos - write
            if ( mode && O_WRONLY )
                pos_p = &s_zrt_channels[handle]->sequential_access_pos;
            else
                pos_p = &s_zrt_channels[handle]->random_access_pos;
            break;
        case RGetRPut:  // random_pos     - read write
            pos_p = &s_zrt_channels[handle]->random_access_pos;
            break;
        default:
            /*do not log if current pos is calculating for logger, to avoid recursion*/
            if ( handle != s_zrt_log_fd ){
                /**DEBUG*******************************************/
                log_msg( "s_manifest->channels[handle].type=" ); log_int( s_manifest->channels[handle].type );
                /**DEBUG*******************************************/
            }
            assert(0);
            break;
        }

        /*do not log if current pos is calculating for logger, to avoid recursion*/
        if ( handle != s_zrt_log_fd ){
            /**DEBUG*******************************************/
            log_msg( "channel_pos handle=" ); log_int( handle );
            log_msg( " current offset=" ); log_int( (int)*pos_p );
            /**DEBUG*******************************************/
        }
        switch ( pos_whence ){
        case EPosGet:
            break;
        case EPosSetRelative:
            if ( !CHECK_NEW_POS(*pos_p+offset) )
                *pos_p += offset;
            else{
                set_zrt_errno( EOVERFLOW );
                return -1;
            }
            break;
        case EPosSetAbsolute:
            if ( !CHECK_NEW_POS(offset) )
                *pos_p = offset;
            else{
                set_zrt_errno( EOVERFLOW );
                return -1;
            }
            break;
        default:
            assert(0);
            break;
        }
        /*do not log if current pos is calculating for logger, to avoid recursion*/
        if ( handle != s_zrt_log_fd && (pos_whence==EPosSetRelative || pos_whence == EPosSetAbsolute) ){
            /**DEBUG*******************************************/
            log_msg( " new offset=" ); log_int( (int)*pos_p ); log_msg( "\n" );
            /**DEBUG*******************************************/
        }

        /*update synthetic size*/
        if ( *pos_p > s_zrt_channels[handle]->maxsize ){
            s_zrt_channels[handle]->maxsize = *pos_p;
        }
        return *pos_p;
    }
    /*do not log if current pos is calculating for logger, to avoid recursion*/
//    if ( handle != s_zrt_log_fd ){
//
//        /**DEBUG*******************************************/
//        log_msg( " default offset=0 \n" );
//        /**DEBUG*******************************************/
//    }
    return 0;
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
    return open_channel( name, flags, mode );
}

/* do nothing but checks given handle */
static int32_t zrt_close(uint32_t *args)
{
    SHOWID;
    errno = 0;
    int handle = (int)args[0];

    /*if valid handle and file was opened previously then perform file close*/
    if ( handle < s_manifest->channels_count &&
            s_zrt_channels[handle] && s_zrt_channels[handle]->open_mode >=0  )
    {
        s_zrt_channels[handle]->random_access_pos = s_zrt_channels[handle]->sequential_access_pos = 0;
        s_zrt_channels[handle]->maxsize = 0;
        s_zrt_channels[handle]->open_mode = -1;
        return 0;
    }
    else{
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
        set_zrt_errno( EBADF );
        return -1;
    }

    /*check if file was not opened for reading*/
    int mode= O_ACCMODE & s_zrt_channels[handle]->open_mode;
    if ( mode != O_RDONLY && mode != O_RDWR  )
    {
        set_zrt_errno( EINVAL );
        return -1;
    }

    int64_t pos = channel_pos(handle, EPosGet, 0);
    if ( CHECK_NEW_POS( pos+length ) != 0 ){
        set_zrt_errno( EOVERFLOW );
        return -1;
    }

    readed = zvm_pread(handle, buf, length, pos );
    if(readed > 0) channel_pos(handle, EPosSetRelative, readed);

    /**DEBUG*******************************************/
    log_msg( "read file handle=" ); log_int( handle );
    log_msg( " readed bytes=" ); log_int( readed );
    log_msg( "\n" );
    /**DEBUG*******************************************/

    ///ZEROVM Obsolete design will removed soon
    if ( readed == -OUT_OF_BOUNDS ){
        return 0;
    }
    if ( readed < 0 ){
        set_zrt_errno( readed );
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

    /*file not opened, bad descriptor*/
    if ( handle < 0 || handle >= s_manifest->channels_count ||
            !s_zrt_channels[handle] ||
            s_zrt_channels[handle]->open_mode < 0  )
    {
        set_zrt_errno( EBADF );
        return -1;
    }

    /*check if file was not opened for writing*/
    int mode= O_ACCMODE & s_zrt_channels[handle]->open_mode;
    if ( mode != O_WRONLY && mode != O_RDWR  )
    {
        set_zrt_errno( EINVAL );
        return -1;
    }

    int64_t pos = channel_pos(handle, EPosGet, 0);
    if ( CHECK_NEW_POS( pos+length ) != 0 ){
        set_zrt_errno( EOVERFLOW );
        return -1;
    }

    wrote = zvm_pwrite(handle, buf, length, pos );
    if(wrote > 0) channel_pos(handle, EPosSetRelative, wrote);

    /**DEBUG*******************************************/
    log_msg( "write file handle=" ); log_int( handle );
    log_msg( " wrote bytes=" ); log_int( wrote );
    log_msg( "\n" );
    /**DEBUG*******************************************/

    if ( wrote < 0 ){
        set_zrt_errno( wrote );
        return -1;
    }
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

    /* check handle */
    if(handle < 0 || handle >= s_manifest->channels_count){
        set_zrt_errno( EBADF );
        return -1;
    }

    /* check non seek-able by zrt std channels*/
    if ( handle == STDIN_FILENO || handle == STDOUT_FILENO || handle == STDERR_FILENO ){
        set_zrt_errno( ESPIPE );
        return -1;
    }

    log_msg( "handle= " ); log_int(handle);
    log_msg( " offset= " ); log_int(offset);
    log_msg( " maxsize= " ); log_int(s_zrt_channels[handle]->maxsize);
    log_msg( "\n" );

    switch(whence)
    {
    case SEEK_SET:
        /**DEBUG*******************************************/
        log_msg( " whence=SEEK_SET " );
        /**DEBUG*******************************************/
        offset = channel_pos(handle, EPosSetAbsolute, offset);
        break;
    case SEEK_CUR:
        /**DEBUG*******************************************/
        log_msg( " whence=SEEK_CUR " );
        /**DEBUG*******************************************/
        offset = channel_pos(handle, EPosSetRelative, offset);
        break;
    case SEEK_END:
        /**DEBUG*******************************************/
        log_msg( " whence=SEEK_END " );
        /**DEBUG*******************************************/
        /*use runtime size instead static size in zvm channel*/
        offset = channel_pos(handle, EPosSetAbsolute, s_zrt_channels[handle]->maxsize + offset );
        break;
    default:
        set_zrt_errno( EPERM ); /* in advanced version should be set to conventional value */
        return -1;
    }

    /* check if new offset is can't be represented by current offset type*/
    if ( offset < 0 )
    {
        set_zrt_errno( EOVERFLOW );
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

/*used by stat, fstat; set stat based on channel type*/
static void set_channel_stat(struct nacl_abi_stat *stat, int handle)
{
    /**DEBUG*******************************************/
    log_msg( "set_channel_stat channel handle= " ); log_int( handle ); log_msg( "\n" );
    /**DEBUG*******************************************/

    /* return stat object */
    stat->nacl_abi_st_dev = 2049;     /* ID of device containing handle */
    stat->nacl_abi_st_ino = 1967;     /* inode number */
    stat->nacl_abi_st_mode = 33261;   /* protection */
    stat->nacl_abi_st_nlink = 1;      /* number of hard links */
    stat->nacl_abi_st_uid = 1000;     /* user ID of owner */
    stat->nacl_abi_st_gid = 1000;     /* group ID of owner */
    stat->nacl_abi_st_rdev = 0;       /* device ID (if special handle) */
    if ( !s_zrt_channels[handle]->maxsize  )
        stat->nacl_abi_st_size = s_manifest->channels[handle].size; /* size in bytes */
    else
        stat->nacl_abi_st_size = s_zrt_channels[handle]->maxsize;

    uint32_t permissions = S_IRUSR | S_IWUSR;
    int temp = 0;
    switch(temp){   /*temporarily fix: instead of chan->type */
    case SGetSPut: /* sequential read, sequential write */
        stat->nacl_abi_st_mode = permissions | S_IFREG;
        stat->nacl_abi_st_blksize = 4096;           /* block size for file system I/O */
        stat->nacl_abi_st_blocks = s_manifest->channels[handle].size/4096+1;   /* number of 512B blocks allocated */
        break;
    case RGetSPut: /* random read, sequential write */
        stat->nacl_abi_st_mode = permissions | S_IFBLK | S_IFREG;
        stat->nacl_abi_st_blksize = 4096;        /* block size for file system I/O */
        stat->nacl_abi_st_blocks =               /* number of 512B blocks allocated */
                ((stat->nacl_abi_st_size + stat->nacl_abi_st_blksize - 1)
                        / stat->nacl_abi_st_blksize) * stat->nacl_abi_st_blksize / 512;
        break;
    case SGetRPut: /* sequential read, random write */
    case RGetRPut: /* random read, random write */
    default:
        assert( 0 ); /*assert because random write not supported yet*/
        break;
    }

    /* files are not allowed to have real date/time */
    stat->nacl_abi_st_atime = 0;      /* time of the last access */
    stat->nacl_abi_st_mtime = 0;      /* time of the last modification */
    stat->nacl_abi_st_ctime = 0;      /* time of the last status change */
    stat->nacl_abi_st_atimensec = 0;
    stat->nacl_abi_st_mtimensec = 0;
    stat->nacl_abi_st_ctimensec = 0;

    /**DEBUG*******************************************/
    log_msg( "size= " ); log_int( stat->nacl_abi_st_size ); log_msg( "\n" );
    /**DEBUG*******************************************/

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
    struct ZVMChannel *channel = NULL;
    int handle;

    /**DEBUG*******************************************/
    log_msg( file ); log_msg( "\n" );
    /**DEBUG*******************************************/

    /* calculate handle number */
    if(file == NULL){
        set_zrt_errno( EFAULT );
        return -1;
    }

    for(handle = 0; handle < s_manifest->channels_count; ++handle)
        if(!strcmp(file, s_manifest->channels[handle].name))
            channel = &s_manifest->channels[handle];

    if ( channel ){
        set_channel_stat( sbuf, handle);
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
    struct ZVMChannel *channel = NULL;

    /* check if user request contain the proper file handle */
    if(handle < 0 || handle >= s_manifest->channels_count){
        set_zrt_errno( EBADF );
        return -1;
    }
    channel = &s_manifest->channels[handle];

    /**DEBUG*******************************************/
    log_msg( "fstat: " ); log_int( handle ); log_msg( ", channel_size=" ); log_int( channel->size ); log_msg( "\n" );
    /**DEBUG*******************************************/

    set_channel_stat( sbuf, handle);
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

SYSCALL_MOCK(getdents, 0)

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
    s_manifest = manifest;
    /*array of pointers*/
    s_zrt_channels = calloc( manifest->channels_count, sizeof(struct ZrtChannelRt*) );
    s_zrt_log_fd = open_channel( ZRT_LOG_NAME, O_WRONLY, 0 );
    /*preallocate sdtin, stdout, stderr channels*/
    open_channel( DEV_STDIN, O_RDONLY, 0 );
    open_channel( DEV_STDOUT, O_WRONLY, 0 );
    open_channel( DEV_STDERR, O_WRONLY, 0 );
}


