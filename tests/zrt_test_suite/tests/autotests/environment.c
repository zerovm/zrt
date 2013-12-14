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

#include <stdio.h>
#include <stdlib.h> //getenv
#include <string.h>
#include <assert.h>
#include "zrt.h"

int main(int argc, char **argv)
{
    extern char **environ;
    char **env = environ;
    printf("environment variables list tests\n");

    printf("\nTEST1: using extern environ:\n");
    while(*env != NULL)
       printf("\"%s\"\n", *env++);

    printf("\nTEST2: using getenv:\n");
    const char* var = NULL;
    const char* val = NULL;
    printf("get environment variables, by getenv:\n");
    var = "SafeWords"; val = getenv(var);  
    if ( val == NULL ) return 1;
    printf("%s=%s\n", var, val );
    assert( !strcmp(val, "klaato_verada_nikto") );

    /*test escaped env var*/
    char* pattern = "1,2,3/\"1,2,3\"";
    var = "usable"; val = getenv(var);  
    printf("%s=%s\n%s\n", var, val,  pattern );
    assert( !strcmp(val,  pattern) );

    printf("\nTEST3: using setenv & getenv:\n");
    printf( "before new environment assignemnt\n" );
    var = "Pum"; val = getenv(var);  
    if ( val != NULL ) return 1;
    printf("%s=%s\n", var, val );
    printf( "setenv status=%d;\nafter new environment assignment\n",  setenv( "Pum", "PumPum", 1) );
    val = getenv(var); 
    if ( val == NULL ) return 1;
    printf("%s=%s\n", var, val );
    return 0;
}
