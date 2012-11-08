/*
 * enum_strings.h
 * string constants, for debugging purposes
 *
 *  Created on: 22.10.2012
 *      Author: YaroslavLitvinov
 */


#ifndef __ENUM_STRINGS_H__
#define __ENUM_STRINGS_H__

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif //_GNU_SOURCE
#include <fcntl.h> //file flags
#include <sys/mman.h>

#define MAX_FLAGS_LEN 1000

enum array_list_t{
    EFileOpenFlags=0,
    EMmapProtFlags,
    EMMapFlags,
    ESeekWhence,
    ELockTypeFlags,
    EFcntlCmd,
    EFileOpenMode
};

#define FILE_OPEN_FLAGS(flags) text_from_flag(flags, EFileOpenFlags)
#define FILE_OPEN_MODE(mode) text_from_id(mode, EFileOpenMode)
#define MMAP_PROT_FLAGS(flags) text_from_flag(flags, EMmapProtFlags)
#define MMAP_FLAGS(flags) text_from_flag(flags, EMMapFlags)
#define SEEK_WHENCE(flags) text_from_id(flags, ESeekWhence)
#define LOCK_TYPE_FLAGS(flags) text_from_flag(flags, ELockTypeFlags)
#define FCNTL_CMD(flags) text_from_id(flags, EFcntlCmd)

/*Get all list of set flags*/
const char* text_from_flag( int flags, enum array_list_t enum_id );
/*Get textual representation of integer id*/
const char* text_from_id( int id, enum array_list_t enum_id );


#endif //__ENUM_STRINGS_H__
