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

/*if path starting with more than one slashes together 
  then return path starting from last of that slashes*/
static const char* absolute_path_starting_with_single_slash(const char* path){
    int i;
    int len = strlen(path);
    /*search last '/' in path starting with several slashes together*/
    for (i=0; i < len; i++){
	/*if it not last item processing*/
	if ( i+1 < len ){
	    if ( path[i] == '/' && path[i+1] != '/' )
		return &path[i];
	}
	/* end of path reached*/
	else
	    return &path[i];
    }
    return path;
}

char* zrealpath( const char* path, char* resolved_path )
{
    /* some applications providing relative path, currently any of zrt filesystems 
     * does not support relative path, so make absolute path just insert '/' into
     * begin of relative path */
    char* absolute_path;
    const char* tmp = NULL; 
    if ( resolved_path != NULL )
	absolute_path = resolved_path;
    else if ( (absolute_path=malloc( strlen(path) + 2 )) == NULL ){
	/*in some cases this code can be called from abort() function due malloc failure,
	  so we need to check malloc result*/
	ZRT_LOG(L_ERROR, "malloc failed, sbrk(0)=%p", sbrk(0) );
	return NULL;
    }
    /*transform . path into root /  */
    if ( strlen(path) == 1 && path[0] == '.' ){
        strcpy( absolute_path, "/\0" );
    }
    /*transform ./ path into root / */
    else if ( strlen(path) == 2 && path[0] == '.' && path[1] == '/' ){
        strcpy( absolute_path, "/\0" );
    }
    /*if relative path is detected then transform it to absolute*/
    else if ( strlen(path) > 1 && path[0] != '/' ){
        strcpy( absolute_path, "/\0" );
        strcat(absolute_path, path);
    }
    /*normalization of global path
     *if path has more than one '/' slashes together then path should be used
     *as absolute path starting from single '/' slash */
    else if ( (tmp=absolute_path_starting_with_single_slash(path)) != path ){
	strcpy(absolute_path, tmp);
    }
    else{
        strcpy(absolute_path, path);
    }
    return absolute_path;
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

