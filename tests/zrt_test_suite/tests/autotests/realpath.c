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


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include "helpers/utils.h"

#include "macro_tests.h"

static char* res;
static char resolved_path[PATH_MAX];

void test_relative_path(const char* relative_path, const char* abs_path){
    int ret;
    fprintf(stderr, "relative=%s, ", relative_path);

    CHECK_PATH_EXISTANCE(relative_path);

    TEST_OPERATION_RESULT( realpath( relative_path, resolved_path),
			   &res, res!=NULL );

    fprintf(stderr, "resolved=%s expected=%s\n", resolved_path, abs_path);
    
    /*realpath & abspath must be equal*/
    TEST_OPERATION_RESULT(strcmp(res, abs_path), &ret, ret==0);
    /*check file existance using abs_path*/
    CHECK_PATH_EXISTANCE(abs_path);
}

int main(int argc, char **argv)
{
    /*test bad cases*/
    TEST_OPERATION_RESULT( realpath( NULL, resolved_path),
			   &res, res==NULL&&errno==EINVAL );

    TEST_OPERATION_RESULT( realpath( "", resolved_path),
			   &res, res==NULL&&errno==ENOENT );

    /*create dir, file not using absolute name, note filenam is TEST_FILE+1*/
    CREATE_EMPTY_DIR("/dev/mount/../../dir"); //it means /dir
    CREATE_FILE("/foo1", "some data", sizeof("some data"));

    /*test relative path, all path components must be exist*/
    test_relative_path("/dir/../foo1", "/foo1");
    test_relative_path("foo1", "/foo1");
    test_relative_path("/dir/../foo1", "/foo1");
    test_relative_path("/dev/..", "/");
    test_relative_path("/dev/.", "/dev");
    test_relative_path("/dev/../", "/");
    test_relative_path("/dev/./", "/dev");
    test_relative_path("/dev/../dev", "/dev");
    test_relative_path("/dev/../dev/", "/dev");
    test_relative_path("/dev/mount/../nvram", "/dev/nvram");

    return 0;
}


