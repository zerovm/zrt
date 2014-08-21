/*
 *
 * Copyright (c) 2014, LiteStack, Inc.
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

#include <stddef.h> //size_t 
#include <unistd.h> //ssize_t
#include "zrt_defines.h" //INSTANCE_L

/*name of constructor*/
#define DIRENT_ENGINE get_dirent_engine

#if (defined __native_client__ || defined __ZRT_HOST) && !defined(__ZRT_SO)
# define ZRTDIRENT
#endif

#ifdef ZRTDIRENT
#define DIRENT struct dirent
#else
#ifndef NAME_MAX
#define NAME_MAX 255
#endif
struct linux_dirent {
    unsigned long  d_ino;     /* Inode number */
    unsigned long  d_off;     /* Offset to next linux_dirent */
    unsigned short d_reclen;  /* Length of this linux_dirent */
    char           d_name[];  /* Filename (null-terminated) */
};
#define DIRENT struct linux_dirent
#endif //ZRTDIRENT

struct DirentEnginePublicInterface{
    size_t (*adjusted_dirent_size)(int d_name_len);
    /*@return size of added dirent, -1 if no space in buf*/
    ssize_t (*add_dirent_into_buf)( char *buf, 
				    int buf_size, 
				    unsigned long d_ino, 
				    unsigned long d_off,
				    unsigned long mode,
				    const char *d_name );
    /*@return item, or NULL if no more items*/
    const char* (*get_next_item_from_dirent_buf)(char *buf, int buf_size, int *cursor, 
					 unsigned long *d_ino, 
					 unsigned long *d_type );
};


struct DirentEnginePublicInterface* get_dirent_engine();
