/*
 * parse_path.c
 *
 *  Created on: 26.09.2012
 *      Author: yaroslav
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include "zrtlog.h"
#include "parse_path.h"


static char s_cached_full_path[4096] = "";


/* check path directory is cached or not.
 * it's extract part related to full directory name from path and compare it
 * to previously cached dir name that's already created on filesystem.
 * @param path to check
 * @return 0 if cached, 1 if not;
 *  */
int create_dir_and_cache_name( const char* dirpath, int len ){
    int res = strncmp( dirpath, s_cached_full_path, len) == 0? 0: 1;
    if ( res != 0 ){
        /*reset old cache and save path in cache*/
	memset(s_cached_full_path, '\0', sizeof(s_cached_full_path));
        strncpy( s_cached_full_path, dirpath, len );
	/* create dir*/
	int ret = mkdir( s_cached_full_path, S_IRWXU );
	ZRT_LOG(L_EXTRA, "mkdir ret=%d: %s", ret, s_cached_full_path);
    }
    else{
	ZRT_LOG(L_EXTRA, "already created dir: %s(len=%d)", dirpath, len);
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
	/*new dir was created*/
        int len = strlen(path);
	/*handle subdir*/
        int count = process_subdirs_via_callback( observer, path, len );
        return count;
    }
    else{
        ZRT_LOG(L_INFO, "found in cache path=%s", path);
        return 0; /*path is found in cache it should not be processed*/
    }
}



