/*
 * enum_strings.h
 * routines to get textual representations from integer values, for debugging purposes
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
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
    EFileOpenMode,
    EArchEntryType
};

#define STR_FILE_OPEN_FLAGS(flags) text_from_flag(flags, EFileOpenFlags)
#define STR_FILE_OPEN_MODE(mode)   text_from_id((mode), EFileOpenMode)
#define STR_MMAP_PROT_FLAGS(flags) text_from_flag(flags, EMmapProtFlags)
#define STR_MMAP_FLAGS(flags)      text_from_flag(flags, EMMapFlags)
#define STR_SEEK_WHENCE(flags)     text_from_id(flags, ESeekWhence)
#define STR_LOCK_TYPE_FLAGS(flags) text_from_flag(flags, ELockTypeFlags)
#define STR_FCNTL_CMD(flags)       text_from_id(flags, EFcntlCmd)
#define STR_ARCH_ENTRY_TYPE(flags) text_from_id(flags, EArchEntryType)

/*Get all list of set flags*/
const char* text_from_flag( int flags, enum array_list_t enum_id );
/*Get textual representation of integer id*/
const char* text_from_id( int id, enum array_list_t enum_id );


#endif //__ENUM_STRINGS_H__
