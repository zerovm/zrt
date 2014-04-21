/*
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

#define _GNU_SOURCE

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"
#include "path_utils.h"
#include "parse_path.h"


static char s_cached_full_path[4096] = "";


int mkpath_recursively(const char* file_path, mode_t mode) {
    assert(file_path && *file_path);
    struct stat st;

    if ( stat(file_path, &st) != 0 ){
	int temp_cursor, result_len, mkdirerr;
	const char* subpath;
	INIT_TEMP_CURSOR(&temp_cursor);

	errno=0;
	while( (subpath=path_subpath_forward(&temp_cursor, file_path, &result_len)) != NULL ){
	    if ( result_len>1 ){
		if ( (mkdirerr=mkdir( strndupa(subpath, result_len), mode)) == 0 || 
		     mkdirerr==-1&&errno==EEXIST )
		    continue;
	    }
	}
	return mkdirerr;
    }
    else{
	errno = EEXIST;
	return -1;
    }
}

/* check path directory is cached or not.
 * it's extract part related to full directory name from path and compare it
 * to previously cached dir name that's already created on filesystem.
 * @param path to check
 * @return 0 if cached, -1 if not;
 *  */
int create_dir_and_cache_name( const char* dirpath, int len ){
    int res = strncmp( dirpath, s_cached_full_path, len) == 0? 0: 1;
    if ( res != 0 ){
        /*reset old cache and save path in cache*/
	memset(s_cached_full_path, '\0', sizeof(s_cached_full_path));
        strncpy( s_cached_full_path, dirpath, len );
	/* create dir*/
	int ret = mkdir( s_cached_full_path, S_IRWXU );
	if ( ret != 0 && errno != EEXIST ){
	    /*error while creating dir, new path handled, cache not saved, 
	     *it is needed to create sub dir previously*/
	    memset(s_cached_full_path, '\0', sizeof(s_cached_full_path));
	    res = -1; /*new path handled, cache not saved*/
	}
	else{
	    /*directory exist*/
	    res = 0;
	}
	ZRT_LOG(L_EXTRA, "mkdir errno=%d, ret=%d: %s", errno, ret, s_cached_full_path);
    }
    else{
	/*path already handled*/
    }
    return res;
}


/*Search rightmost '/' and return substring*/
static int process_subdirs_via_callback( struct ParsePathObserver* observer, const char *path, int len ){
    int ret = 0;
    /*locate rightmost '/' inside path
     * can't use strrchr for char array path with len=length because path[len] not null terminated char*/
    const char *dirpath = NULL;
    int i;
    for(i=len-1; i >=0; i--){
        if(path[i] == '/'){
            dirpath = &path[i];
            break;
        }
    }
    if ( dirpath ){
        int subpathlen = dirpath-path;
        /*if matched root path '/' it should be processed as '/'*/
        if ( dirpath[0]=='/' && len > 1 && !subpathlen ){
            subpathlen = 1;
        }
        process_subdirs_via_callback( observer, path, subpathlen );
	/*callback_parse should be guarantied that all nested dirs created*/
        (*observer->callback_parse)(observer, path, subpathlen);
        ++ret;
    }
    return ret;
}


int parse_path( struct ParsePathObserver* observer, const char *path ){
    assert(observer); /*observer struct should exist*/
    assert(observer->callback_parse);  /*observer function should exist*/
    
    /*extract dirname from filename*/
    char *c = strrchr(path, '/');
    int dir_path_len = (int)(c-path)+1;
    /*to be sure - check len validity and set actual len*/
    if ( dir_path_len <= 0 )
	dir_path_len = strlen(path);

    if ( create_dir_and_cache_name(path, dir_path_len) != 0 ){
	/*new dir was created, handle subdir*/
        int count = process_subdirs_via_callback( observer, path, dir_path_len );
        return count;
    }
    else{
        ZRT_LOG(L_INFO, "found in cache path=%s", path);
        return 0; /*path is found in cache it should not be processed*/
    }
}



