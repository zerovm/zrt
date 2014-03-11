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

#define ROOT_PATH "/"
#define NEWDIR_PATH "/newdir"
#define FILE_PATH "/filepath"
#define WRONG_PATH "/wrongpath"
#define DEV_PATH "/dev"

#define SMALL_BUF_SIZE 2

void test_issue_93(){
    #define STR "1111111"
    int ret;
    TEST_OPERATION_RESULT( chdir("/tmp"), &ret, ret==0&&errno==0 );
    CREATE_FILE( "@tmp_1", STR, strlen(STR) );
    CHECK_PATH_EXISTANCE("/tmp/@tmp_1");
}


int main(int argc, char **argv)
{
    char buf[PATH_MAX];
    int ret;
    char* res;
    
    /*some preparations*/
    CREATE_FILE(FILE_PATH, "abcd", 4);

    /*test wrong cases*/
    res = getcwd(buf, 0);
    TEST_OPERATION_RESULT( getcwd(buf, 0), &res, res==0&&errno==EINVAL );
    TEST_OPERATION_RESULT( chdir(WRONG_PATH), &ret, ret==-1&&errno==ENOENT );
    TEST_OPERATION_RESULT( chdir(FILE_PATH), &ret, ret==-1&&errno==ENOTDIR );
    /*test ERANGE error*/
    TEST_OPERATION_RESULT( mkdir(WRONG_PATH, 0777), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( chdir(WRONG_PATH), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( getcwd(buf, SMALL_BUF_SIZE), &res, res==NULL&&errno==ERANGE );

    /*test unlink for cur dir*/
    TEST_OPERATION_RESULT( mkdir(NEWDIR_PATH, 0777), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( chdir(NEWDIR_PATH), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( rmdir(NEWDIR_PATH), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( getcwd(buf, PATH_MAX), &res, !res&&errno==ENOENT );
    
    /*test correct cases*/
    TEST_OPERATION_RESULT( chdir(ROOT_PATH), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( getcwd(buf, PATH_MAX), &res, res!=NULL&&!strcmp(res, ROOT_PATH)&&errno==0 );

    /*prepare access file in current directory*/
    TEST_OPERATION_RESULT( mkdir(NEWDIR_PATH, 0777), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( chdir(NEWDIR_PATH), &ret, ret==0&&errno==0 );
    sprintf(buf, "%s/newfile", NEWDIR_PATH);
    CREATE_FILE(buf, "abcd", 4);
    /*access file in current directory*/
    CHECK_PATH_EXISTANCE("newfile");

    TEST_OPERATION_RESULT( get_current_dir_name(), &res, res!=NULL&&!strcmp(res, NEWDIR_PATH)&&errno==0 );
    free(res);

    /*test chdir with channels filesystem*/
    TEST_OPERATION_RESULT( chdir(DEV_PATH), &ret, ret==0&&errno==0 );
    CHECK_PATH_EXISTANCE("stdin");

    test_issue_93();
    return 0;
}


