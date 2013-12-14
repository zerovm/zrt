/*
 * rmdir test
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
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "macro_tests.h"

int main(int argc, char **argv)
{
    int ret;
    struct stat st;
    char* nullstr = NULL;

    TEST_OPERATION_RESULT(
			  rmdir(nullstr),
			  &ret, ret==-1&&errno==EINVAL );
    TEST_OPERATION_RESULT(
			  rmdir("nonexistent"),
			  &ret, ret==-1&&errno==ENOENT );
    CREATE_NON_EMPTY_DIR(DIR_NAME, DIR_FILE);
    TEST_OPERATION_RESULT(
			  stat(DIR_NAME, &st),
			  &ret, ret==0&&st.st_nlink==2 );
    TEST_OPERATION_RESULT(
			  rmdir("/dev/../"DIR_NAME),
			  &ret, ret==-1&&errno==ENOTEMPTY );

    #define FULL_DIR_FILE DIR_NAME "/" DIR_FILE
    TEST_OPERATION_RESULT(
			  rmdir(FULL_DIR_FILE),
			  &ret, ret==-1&&errno==ENOTDIR );

    CREATE_EMPTY_DIR(DIR_NAME2);
    TEST_OPERATION_RESULT(
			  rmdir(DIR_NAME2),
			  &ret, ret==0 );

    return 0;
}


