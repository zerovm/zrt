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

#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "path_utils.h"

int is_relative_path(const char *path){
    int cursor, reslen;
    INIT_TEMP_CURSOR(&cursor);
    const char *res = path_component_forward(&cursor, path, &reslen);
    if ( res == NULL || res[0] != '/' ) return 1;
    while( (res=path_component_forward(&cursor, path, &reslen)) != NULL){
	if ( !strncmp("..", res, reslen) ) return 1;
    }
    return 0;
    
}

static const char* locate_last_occurence(const char *str, int len, char c ){
    /* locate rightmost char inside string, can't use strrchr because
     * it's no guarantied that str is null terminated string*/
    int i;
    for(i=len-1; i >=0; i--){
        if(str[i] == c ){
            return &str[i];
        }
    }
    return NULL;
}

static const char* locate_first_occurence(const char *str, int len, char c ){
    /* locate keftmost char inside string, can't use strchr because
     * it's no guarantied that str is null terminated string*/
    int i;
    for(i=0; i <len; i++){
        if(str[i] == c ){
            return &str[i];
        }
    }
    return NULL;
}


const char *path_component_forward(int *temp_cursor, const char *path, int *result_len){
    assert(temp_cursor);
    const char* component_start; 
    int pathlen = strlen(path);
    if ( *temp_cursor == DEFAULT_TEMP_CURSOR_VALUE ) //expected to start walk through path
	*temp_cursor = 0;

    const char *currentpath = path+*temp_cursor;
    int currentpathlen = pathlen-*temp_cursor;

    /*look for component*/
    component_start = locate_first_occurence( currentpath, currentpathlen, '/' );
    if ( component_start != NULL ){
	/*get component resides before slash*/
	*result_len = (int)(component_start-currentpath);
    }
    /*NULL component_start - last component located*/
    else if ( *temp_cursor <= pathlen ){
	*result_len= currentpathlen;
    }
    else
	*result_len = 0;

    if ( *result_len > 0 )
	component_start = currentpath;
    *temp_cursor += *result_len +1;

    /*empty component, like a root*/
    if ( component_start && *result_len == 0 && path[0]=='/' ){
	*result_len=1;
    }
    return component_start;
}

const char *path_component_backward(int *temp_cursor, const char *path, int *result_len){
    assert(temp_cursor);
    const char* last_component_start; 
    int proceed=1;
    int pathlen = strlen(path);
    if ( *temp_cursor == DEFAULT_TEMP_CURSOR_VALUE ) //expected to start walk through path
	*temp_cursor = pathlen;

    while( proceed != 0 ){
	/*look for component*/
	last_component_start = locate_last_occurence( path, *temp_cursor, '/' );
	if ( last_component_start != NULL ){
	    /*get component resides after slash*/
	    *result_len = *temp_cursor - (int)(last_component_start-path) -1;
	    *temp_cursor -= *result_len +1;
	    ++last_component_start;
	}
	/*NULL component - check if root slash located*/
	else if ( *temp_cursor == 0 && path[0] == '/' ){
	    *result_len=1;
	    last_component_start = path;
	    *temp_cursor = -1;
	    proceed = 0;
	}

	/*trailing slash with an empty component*/
	if ( last_component_start && *result_len == 0 ){
	    ;  //continue
	}
	else
	    proceed = 0;
    }
    return last_component_start;
}

const char *path_subpath_forward(int *temp_cursor, const char *path, int *result_len){
    int component_len;
    const char *component =path_component_forward( temp_cursor, path, &component_len);
    if( component != NULL ){
	*result_len = component-path+component_len;
	return path;
    }
    else
	return NULL;
}

const char *path_subpath_backward(int *temp_cursor, const char *path, int *result_len){
    int component_len;
    const char *last_component =path_component_backward( temp_cursor, path, &component_len);
    if( last_component != NULL ){
	*result_len = last_component-path+component_len;
	return path;
    }
    else
	return NULL;
}


int test_function(const char *(*function)(int *temp_cursor, const char *path, int *result_len),
		  const char *path, const char **test_vector, int test_vector_len){
    int temp_cursor, result_len;
    const char* result;
    const char *expected;
    int i;

    INIT_TEMP_CURSOR(&temp_cursor);
    for ( i=0; i < test_vector_len; i++ ){
	result = function( &temp_cursor, path, &result_len);
	expected= test_vector[i];
	if ( expected == NULL )
	    assert(result == NULL );
	else
	    assert(result != NULL && result_len==strlen(expected) && !strncmp(expected, result, strlen(expected)) );
    }
    return 0;
}


int test_path_utils(){
    int temp_cursor, result_len;
    char path[] = "/1/22/";
    const char* result;
    char *expected;

    const char *component_backward_test0[] = { "/", NULL };
    test_function(path_component_backward, "/", component_backward_test0, 2);

    const char *component_backward_test1[] = { "22", "1", "/", NULL };
    test_function(path_component_backward, "/1/22/", component_backward_test1, 4);

    const char *component_backward_test2[] = { "22", "1", "/", NULL };
    test_function(path_component_backward, "/1/22", component_backward_test2, 4);

    const char *component_subpath_test1[] = { "/1/22", "/1", "/", NULL };
    test_function(path_subpath_backward, "/1/22/", component_subpath_test1, 4);

    const char *component_subpath_test2[] = { "/1/22", "/1", "/", NULL };
    test_function(path_subpath_backward, "/1/22", component_subpath_test2, 4);

    const char *component_fsubpath_test1[] = { "/", "/1", "/1/22", NULL };
    test_function(path_subpath_forward, "/1/22/", component_fsubpath_test1, 4);

    const char *component_fsubpath_test2[] = { "/", "/1", "/1/22", NULL };
    test_function(path_subpath_forward, "/1/22", component_fsubpath_test2, 4);

    /*******/
    const char *component_forward_test0[] = { "/", NULL };
    test_function(path_component_forward, "/", component_forward_test0, 2);

    const char *component_forward_test1[] = { "/", "1", "22", NULL };
    test_function(path_component_forward, "/1/22/", component_forward_test1, 4);

    const char *component_forward_test2[] = { "/", "1", "22", NULL };
    test_function(path_component_forward, "/1/22", component_forward_test2,4 );
    return 0;
}
