/*
 * path_utils.c
 *
 *  Created on: 25.12.2012
 *      Author: YaroslavLitvinov
 */

#include <string.h>
#include <stdlib.h>

#include "path_utils.h"

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
    else{
        strcpy(absolute_path, path);
    }
    return absolute_path;
}
