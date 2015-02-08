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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "macro_tests.h"

#define JOIN_STR(a,b) a b
#define FILENAME getenv("FPATH")

int main(int argc, char**argv){
    char path[PATH_MAX];
    int ret;
    struct stat st;

    /*testing import errors*/
    CHECK_PATH_EXISTANCE("/test/test.1234");
    /*this is a dir, one of tar image imports has error while injecting this file*/
    TEST_OPERATION_RESULT( stat("/test/test.1234", &st), 
			   &ret, ret==0&&S_ISDIR(st.st_mode));
    CHECK_PATH_EXISTANCE("/test/test.1234/test.1234");
    /*this is a file, one of tar image imports expected as failed,
      mountpoint not created, because of existing file*/
    TEST_OPERATION_RESULT( stat("/test/test.1234/test.1234", &st), 
			   &ret, ret==0&&S_ISREG(st.st_mode));
    CHECK_PATH_NOT_EXIST("/test/test.1234/bad");
    
    CHECK_PATH_EXISTANCE("/warm");
    sprintf(path, "/warm/%s", FILENAME );
    CHECK_PATH_EXISTANCE( path );

    //files injected twice here, need to check files validity
    CHECK_PATH_EXISTANCE("/ok");
    sprintf(path, "/ok/%s", FILENAME );
    CHECK_PATH_EXISTANCE( path );

    /*file size from /warm folder is different than a file from /ok dir
     because /warm folder was mounted once in according to removable=no,
     and file in /ok folder was updated.*/
    int sz1,sz2;
    sprintf(path, "/warm/%s", FILENAME );
    GET_FILE_SIZE(path, &sz1);

    sprintf(path, "/ok/%s", FILENAME );
    GET_FILE_SIZE(path, &sz2);
    fprintf(stderr, "sz1=%d, sz2=%d\n", sz1, sz2);
    TEST_OPERATION_RESULT( sz1!=sz2,
    			   &ret, ret!=0);

    /*this mountpoint was mounted without explicit specifying of
      'removable' option as optional with default value removable=no*/
    CHECK_PATH_EXISTANCE("/ok1");
    CHECK_PATH_NOT_EXIST("/bad2");
    return 0;
}

