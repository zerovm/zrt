/*
 *
 * Copyright (c) 2013, LiteStack, Inc.
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


#include <string.h>
#include <stdlib.h>
#include <unistd.h> //sbrk
#include <limits.h>

#include "zrtlog.h"
#include "utils.h"

#define MERGE_PATH_COMPONENTS(path1, path2, result){		\
	int path1len = strlen(path1);				\
	strcpy(result, path1);					\
	if ( path1len > 0 && path1[path1len-1] != '/' &&	\
	     strlen(path2) > 0 && path2[0] != '/' ){		\
	    strncpy(result+path1len, "/", PATH_MAX-path1len);	\
	    ++path1len;						\
	}							\
	strncpy(result+path1len, path2, PATH_MAX-path1len);	\
    }


char* zrealpath( const char* path, char* resolved_path )
{
    char temp_path[PATH_MAX];
    char* last_component;
    char* absbasepath;
    char* res=NULL;
    /*exclude last path component from path param*/
    last_component=strrchr(path, '/');
    /*if path it's a dir without trailing backslash '/' */
    if ( last_component!= NULL && 
	 (!strcmp(last_component, "/..") || !strcmp(last_component, "/.")) ){
	if ( (absbasepath = realpath(path, resolved_path)) != NULL )
	    res = resolved_path;
    }
    /*if path has trailing backslash and it's not a root*/
    else if ( last_component != path && last_component!= NULL ){
	int basepathlen = (int)(last_component - path);
	strncpy(temp_path, path, basepathlen);
	if ( basepathlen < PATH_MAX )
	    temp_path[basepathlen] = '\0';

	/*combine absolute base path and last path component*/
	if ( (absbasepath = realpath(temp_path, resolved_path)) != NULL ){
	    MERGE_PATH_COMPONENTS(absbasepath, last_component, resolved_path);
	    res = resolved_path;
	}
    }
    /*if path has no trailing backslash*/
    else if ( last_component == NULL ){
	char* rootpath = getcwd(temp_path, PATH_MAX);
	MERGE_PATH_COMPONENTS(rootpath, path, resolved_path);
	res = resolved_path;
    }
    else{
	strcpy(resolved_path, path);
	res = resolved_path;
    }

    return res;
}


uint strtouint_nolocale(const char* str, int base, int *err ){
    #define CURRENT_CHAR str[idx]
    int idx;
    uint delta;
    int numlen = strlen(str);
    uint res = 0;
    uint append=1;
    for ( idx=numlen-1; idx >= 0; idx-- ){
	if ( CURRENT_CHAR >= '0' && CURRENT_CHAR <= '9' ){
	    delta = append* (uint)(CURRENT_CHAR - '0');
	    if ( !(delta > UINT_MAX-res) )
		res += delta;
	    else{
		res=0;
		*err = 1;
		return 0;
	    }
	    append *= base;
	}
    }
    return res;
}

int test_strtouint_nolocale(){
    int err=0;
    uint res;
    res = strtouint_nolocale("0", 10, &err );
    if ( res!=0 || err!=0 ) return -1;

    res = strtouint_nolocale("4294967295", 10, &err );
    if ( res!=4294967295 || err!=0 ) return -1;

    res = strtouint_nolocale("4294967297", 10, &err );
    if ( res!=0 || err==0 ) return -1;

    return 0;
}
