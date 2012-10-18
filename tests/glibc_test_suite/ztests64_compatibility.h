#ifndef __ZTESTS_64_COMPATIBILITY_H__
#define __ZTESTS_64_COMPATIBILITY_H__

#define off64_t  off_t
#define ftello64 ftell
#define stat64   stat
#define fopen64  fopen
#define fseeko64 fseek
#define lseek64  lseek
#define rlimit64 rlimit
#define mkstemp64 mkstemp
#define getrlimit64 getrlimit
#define setrlimit64 setrlimit
#define ftw64    ftw

//filename: "ztests64_compatibility.h"

#endif //__ZTESTS_64_COMPATIBILITY_H__
