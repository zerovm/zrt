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

/* same here */
struct nacl_abi_timeval {
    int64_t   nacl_abi_tv_sec;
    int32_t   nacl_abi_tv_usec;
};


#endif /* NACL_STRUCT_H_ */
