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

    CHECK_PATH_EXISTANCE("/warm");
    sprintf(path, "/warm/%s", FILENAME );
    CHECK_PATH_EXISTANCE( path );

    CHECK_PATH_NOT_EXIST("/bad");
    sprintf(path, "/bad/%s", FILENAME );
    CHECK_PATH_NOT_EXIST( path );

    //files injected twice here, need to check files validity
    CHECK_PATH_EXISTANCE("/ok");
    sprintf(path, "/ok/%s", FILENAME );
    CHECK_PATH_EXISTANCE( path );

    //check file contents if injecting one of file that already exists
    int sz1,sz2,ret;
    sprintf(path, "/warm/%s", FILENAME );
    GET_FILE_SIZE(path, &sz1);

    sprintf(path, "/ok/%s", FILENAME );
    GET_FILE_SIZE(path, &sz2);
    TEST_OPERATION_RESULT( sz1==sz2,
    			   &ret, ret!=0);

    CHECK_PATH_NOT_EXIST("/bad1");
    CHECK_PATH_NOT_EXIST("/bad2");
    return 0;
}

