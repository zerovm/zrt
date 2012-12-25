/*
 * realpath.c
 * realpath implementation that substitude glibc stub implementation
 *
 *  Created on: 24.12.2012
 *      Author: yaroslav
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "path_utils.h"

char *realpath(const char *path, char *resolved_path)
{
    char* result_path = NULL;
    ZRT_LOG(L_SHORT, "path=%s, resolved_path=%p", path, resolved_path);
    /*argument path is valid*/
    if ( path != NULL ){
	result_path = alloc_absolute_path_from_relative(path);
	/*if buffer not provided then it should be allocated dynamically*/
	if ( resolved_path == NULL ){
	    /*result_path should be freed after using*/
	}
	else if ( strlen(result_path) < PATH_MAX ){
	    strcpy(resolved_path, result_path);
	    free(result_path);
	    result_path = resolved_path;
	}
    }
    else{	
	ZRT_LOG(L_ERROR, P_TEXT, "Bad input path == NULL.");
	SET_ERRNO(ENOENT);
    }

    return result_path;
}
