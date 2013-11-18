/*
 * utils.c
 *
 *  Created on: 25.12.2012
 *      Author: YaroslavLitvinov
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h> //sbrk
#include <limits.h>

#include "zrtlog.h"
#include "utils.h"

#define MERGE_PATH_COMPONENTS(path1, path2, result){		\
	int path1len = strlen(path1);				\
	strcpy(result, path1);					\
	strncpy(result+path1len, path2, PATH_MAX-path1len);	\
    }


char* zrealpath( const char* path, char* resolved_path )
{
    char temp_path[PATH_MAX];
    char* last_component;
    char* absbasepath;
    /*exclude last path component from path param*/
    /**/
    //    do{
    last_component=strrchr(path, '/');
    //    }
    //    while ( last_component==(path+pathlen) && last_component!=path )
    /*if last_component resides in root*/
    if ( last_component != path && last_component!= NULL ){
	int basepathlen = (int)(last_component - path);
	strncpy(temp_path, path, basepathlen);
	if ( basepathlen < PATH_MAX )
	    temp_path[basepathlen] = '\0';

	/*combine absolute base path and last path component*/
	if ( (absbasepath = realpath(temp_path, resolved_path)) != NULL ){
	    MERGE_PATH_COMPONENTS(absbasepath, last_component, resolved_path);
	    return resolved_path;
	}
	else 
	    return NULL;
    }
    else if ( last_component == NULL ){
	char* rootpath = getcwd(temp_path, PATH_MAX);
	MERGE_PATH_COMPONENTS(rootpath, path, resolved_path);
	return resolved_path;
    }
    else
	return path;

    return resolved_path;
}


uint strtouint_nolocale(const char* str, int base, int *err ){
    #define CURRENT_CHAR str[idx]
    int idx;
    int numlen = strlen(str);
    uint res = 0;
    uint append=1;
    for ( idx=numlen-1; idx >= 0; idx-- ){
	if ( CURRENT_CHAR >= '0' && CURRENT_CHAR <= '9' ){
	    if ( (res + append* (uint)(CURRENT_CHAR - '0')) < UINT_MAX )
		res += append* (uint)(CURRENT_CHAR - '0');
	    else{
		*err = 1;
		return 0;
	    }
	    append *= base;
	}
    }
    return res;
}

