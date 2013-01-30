/*
 * parse_path.c
 *
 *  Created on: 26.09.2012
 *      Author: yaroslav
 */

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
static int is_cache_matched( const char* path ){
    int res;
    int base_dir_path_len;
    /*extract full directory name up to last symbol '/' and compare it to cache*/
    char *c = strrchr(path, '/');
    base_dir_path_len = (int)(c-path)+1;
    if ( base_dir_path_len == 1 /*root matched*/ ){
	/*root always exist and should not be handled*/
	res = 1; 
    }
    else
	res = strncmp( path, s_cached_full_path, base_dir_path_len) == 0? 0: 1;

    if ( res == 0 && base_dir_path_len > 0 && base_dir_path_len < 200 ){
	char g[200];
	memset(g, '\0', 200);
	strncpy(g, path, base_dir_path_len);
	ZRT_LOG(L_EXTRA, "already created dir: %s", g);
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
	/*call it after single item parsed. it can be used for post handling*/
        (*observer->callback_parse)(observer, path, subpathlen);
        ++ret;
    }
    return ret;
}


int parse_path( struct ParsePathObserver* observer, const char *path ){
    assert(observer); /*observer struct should exist*/
    assert(observer->callback_parse);  /*observer function should exist*/

    if ( is_cache_matched(path) != 0 ){
        int len = strlen(path);
	/*handle subdir*/
        int count = process_subdirs_via_callback( observer, path, len );
        /*cache path*/
        strncpy( s_cached_full_path, path, len );
        return count;
    }
    else{
        ZRT_LOG(L_INFO, "found in cache path=%s", path);
        return 0; /*path is found in cache it should not be processed*/
    }
}



