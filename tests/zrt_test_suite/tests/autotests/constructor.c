/*
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



#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <limits.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include "macro_tests.h"
#include "parse_path.h"

static int s_test_value;

void constr() __attribute__((constructor));
void constr2() __attribute__((constructor));

void constr()
{
    /*test constructor whatever was called*/
    int ret;
    ++s_test_value;
    int res = fprintf(stderr, "constr value=%d\n", s_test_value);
    TEST_OPERATION_RESULT(res, &ret, ret>0);
    const char* envvar_testdir = getenv("TESTDIR");
    TEST_OPERATION_RESULT(envvar_testdir!=NULL, &ret, ret==1);
    /*create directory to be access it at the main, relation of
      filesystem and constructor*/
    mkpath_recursively(envvar_testdir, 0666);
}

void constr2()
{
    /*test constructor whatever was called*/
    int ret;
    ++s_test_value;
    int res = fprintf(stderr, "constr2 value=%d\n", s_test_value);
    TEST_OPERATION_RESULT(res, &ret, ret>0);
}

void destr() __attribute__((destructor));

void destr()
{
    int ret;
    TEST_OPERATION_RESULT(--s_test_value, &ret, ret==1);
    fprintf(stderr, "destr value=%d\n", s_test_value);
}

int main(int argc, char **argv)
{
    int ret;
    TEST_OPERATION_RESULT(s_test_value, &ret, ret==2);
    const char* envvar_testdir = getenv("TESTDIR");
    CHECK_PATH_EXISTANCE(envvar_testdir);
    fprintf(stderr, "main value=%d\n", s_test_value);
    return 0;
}

