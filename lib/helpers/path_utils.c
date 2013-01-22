/*
 * path_utils.c
 *
 *  Created on: 25.12.2012
 *      Author: YaroslavLitvinov
 */

#include <string.h>
#include <stdlib.h>

#include "path_utils.h"


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

/*
 * alloc absolute path, for relative path just insert into beginning '/' char, 
 * for absolute path just alloc and return. user application can provide relative path, 
 * currently any of zrt filesystems does not supported relative path, so making absolute 
 * path is required.
 */
char* alloc_absolute_path_from_relative( const char* path )
{
    /* some applications providing relative path, currently any of zrt filesystems 
     * does not support relative path, so make absolute path just insert '/' into
     * begin of relative path */
    char* absolute_path = malloc( strlen(path) + 2 );
    const char* tmp = NULL; 
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
