/*
 * nacl_struct.h
 *
 *  Created on: 19.09.2012
 *      Author: yaroslav
 */

#ifndef NACL_STRUCT_H_
#define NACL_STRUCT_H_

#include <sys/stat.h>

struct nacl_abi_dirent {

    unsigned long long   d_ino;     /*offsets NaCl 0 */
    unsigned long long  d_off;     /*offsets NaCl 8 */
    uint16_t  d_reclen;  /*offsets NaCl 16 */
    char     d_name[];  /*offsets NaCl 18 */
};

/*
 * temporary fix for nacl. stat != nacl_abi_stat
 * also i had a weird error when tried to use "service_runtime/include/sys/stat.h"
 */
struct nacl_abi_stat
{
    /* must be renamed when ABI is exported */
    int64_t   nacl_abi_st_dev;       /* not implemented */
    uint64_t  nacl_abi_st_ino;       /* not implemented */
    uint32_t  nacl_abi_st_mode;      /* partially implemented. */
    uint32_t  nacl_abi_st_nlink;     /* link count */
    uint32_t  nacl_abi_st_uid;       /* not implemented */
    uint32_t  nacl_abi_st_gid;       /* not implemented */
    int64_t   nacl_abi_st_rdev;      /* not implemented */
    int64_t   nacl_abi_st_size;      /* object size */
    int32_t   nacl_abi_st_blksize;   /* not implemented */
    int32_t   nacl_abi_st_blocks;    /* not implemented */
    int64_t   nacl_abi_st_atime;     /* access time */
    int64_t   nacl_abi_st_atimensec; /* possibly just pad */
    int64_t   nacl_abi_st_mtime;     /* modification time */
    int64_t   nacl_abi_st_mtimensec; /* possibly just pad */
    int64_t   nacl_abi_st_ctime;     /* inode change time */
    int64_t   nacl_abi_st_ctimensec; /* only nacl; possibly just pad */
};
void set_nacl_stat( const struct stat* stat, struct nacl_abi_stat* nacl_stat );


/* same here */
struct nacl_abi_timeval {
    int64_t   nacl_abi_tv_sec;
    int32_t   nacl_abi_tv_usec;
};


#endif /* NACL_STRUCT_H_ */
