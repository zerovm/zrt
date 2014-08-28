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

#include "zrtlog.h"
#include "dirent_engine.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>

/*dirent structure not have constant size it's can be vary depends on name length.       
  also dirent size should be multiple of the 8 bytes, so adjust it*/
#define ZRT_DIRENTLEN(d_name_len)						\
    ROUND_UP( (offsetof(DIRENT, d_name) + d_name_len +1 /* NUL termination */), 8 )

#define	NON_ZRT_DIRENTLEN(d_name_len)	\
    ROUND_UP( (offsetof(DIRENT, d_name) + d_name_len +1 /* NUL termination */ + 1 /*d_type*/), 8 )

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))


static int d_type_from_mode(unsigned int mode){
    switch (mode & S_IFMT) {
    case S_IFBLK:  return DT_BLK;
    case S_IFCHR:  return DT_CHR;
    case S_IFDIR:  return DT_DIR;
    case S_IFIFO:  return DT_FIFO;
    case S_IFLNK:  return DT_LNK;
    case S_IFREG:  return DT_REG;
    case S_IFSOCK: return DT_SOCK;
    default:       return DT_UNKNOWN;
    }
}

static size_t adjusted_dirent_size(int d_name_len){
#ifdef ZRTDIRENT
    return ZRT_DIRENTLEN(d_name_len);
#else
    return NON_ZRT_DIRENTLEN(d_name_len);
#endif //ZRTDIRENT
}

/*low level function, copy dirent args into buf*/
static ssize_t put_dirent_into_buf( char *buf, 
				   int buf_size, 
				   unsigned long d_ino, 
				   unsigned long d_off,
				   unsigned long mode,
				   const char *d_name ){
    DIRENT *dirent = (DIRENT *) buf;
    ZRT_LOG(L_EXTRA, "dirent offset: ino_off=%u, off_off=%u, reclen_off=%u, name_off=%u",
            offsetof( DIRENT, d_ino ),
            offsetof( DIRENT, d_off ),
            offsetof( DIRENT, d_reclen ),
            offsetof( DIRENT, d_name ) );

    int namelength = strlen(d_name);
    size_t adjusted_size = adjusted_dirent_size(namelength);

    /*if size of the current dirent data is less than available buffer size
     then fill it by data*/
    if ( adjusted_size <= buf_size ){
        dirent->d_reclen = adjusted_size;
        dirent->d_ino = d_ino;
#ifdef ZRTDIRENT
	dirent->d_type = d_type_from_mode(mode);
#else
	buf[adjusted_size-1] = d_type_from_mode(mode);
#endif //ZRTDIRENT
        if ( d_off == 0x7fffffff )
            dirent->d_off = 0x7fffffff;
        else
            dirent->d_off = d_off+dirent->d_reclen;

        memcpy( dirent->d_name, d_name, namelength );
        ((char*)dirent->d_name)[namelength] = '\0';

        ZRT_LOG(L_SHORT, "dirent: name=%s, ino=%u, d_off=%u, d_reclen=%d "
#ifdef ZRTDIRENT
		", d_type=%d"
#endif
                ,d_name, 
		(unsigned int)d_ino, 
		(unsigned int)d_off, 
		dirent->d_reclen
#ifdef ZRTDIRENT
		,dirent->d_type 
#endif
		);
        return dirent->d_reclen;
    }
    /*buffer is not enough to save current dirent structure*/
    else{
        ZRT_LOG(L_EXTRA, "no enough buffer, "
		"data_size=%d, buf_size=%d", 
		adjusted_size, buf_size);
        return -1; /*no enough buffer size*/
    }
}

static const char *get_next_item_from_dirent_buf(char *buf, int buf_size, int *cursor, 
						 unsigned long *d_ino, 
						 unsigned long *d_type ){
    /*read from buffer which filled by getdents items back*/
    if( (*cursor+adjusted_dirent_size(1)) <= buf_size ){
	DIRENT *dir_item = (DIRENT *)&buf[*cursor];
	*cursor += adjusted_dirent_size( strlen(dir_item->d_name) );
	*d_ino  = dir_item->d_ino;
#ifdef ZRTDIRENT
	*d_type = dir_item->d_type;
#endif //ZRTDIRENT
	return dir_item->d_name;
    }
    else
	return NULL;
}

struct DirentEnginePublicInterface s_dirent_engine = {
    adjusted_dirent_size,
    put_dirent_into_buf,
    get_next_item_from_dirent_buf
};


struct DirentEnginePublicInterface* get_dirent_engine(){
    return &s_dirent_engine;
}

