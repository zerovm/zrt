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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h> //getenv
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>
#include "macro_tests.h"


void zrt_test_issue73()
{
    const char modulepy[] = "/tmp/__main__.py";
    int ret, fd;

    struct stat buffer;
    umask(022);

    TEST_OPERATION_RESULT( open(modulepy, O_WRONLY|O_CREAT|O_EXCL|O_TRUNC, 0100666), &fd, fd!=-1&&errno==0);
    TEST_OPERATION_RESULT( fstat(fd, &buffer), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( buffer.st_mode == 0100644, &ret, ret!=0 );
    TEST_OPERATION_RESULT( unlink(modulepy), &ret, ret==0 );

    TEST_OPERATION_RESULT( open(modulepy, O_WRONLY|O_CREAT|O_EXCL|O_TRUNC, 0100444), &fd, fd!=-1&&errno==0);
    TEST_OPERATION_RESULT( fstat(fd, &buffer), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0 );
    fprintf(stderr, "bitwise operation result = %o, mode=%o\n", 
	    0444 & ~022, 
	    buffer.st_mode );
    TEST_OPERATION_RESULT( buffer.st_mode == 0100444, &ret, ret!=0 );
    TEST_OPERATION_RESULT( unlink(modulepy), &ret, ret==0 );


    assert( (0777&buffer.st_mode) == 0444);

}

int main(int argc, char **argv)
{
    int ret;
    extern char **environ;
    char **env = environ;

    zrt_test_issue73();

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

    var = "Pum"; val = getenv(var);  
    TEST_OPERATION_RESULT( val == NULL, &ret, ret!=0 );
    TEST_OPERATION_RESULT( setenv(var, "PumPum", 1), &ret, ret==0&&errno==0  );
    val = getenv(var);
    TEST_OPERATION_RESULT( val!=NULL && !strcmp("PumPum", val), &ret, ret!=0&&errno==0 );
    return 0;
}
