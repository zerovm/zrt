/*
 * fdopen test
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

#define READ_WRITE_CHANNEL "/dev/read-write"
static char tmpfile_template[PATH_MAX] = "/tmp/test.XXXXXX";

/*pread/pwrite tests*/
void test_zrt_issue_81(const char* filename);

/*dup-dup2 tests*/
void test_zrt_issue_78(const char* filename);

int main(int argc, char **argv)
{
    int fd;
    /*devices fs test pread/pwrite*/
    test_zrt_issue_81(READ_WRITE_CHANNEL);

    errno = 0;
    TEST_OPERATION_RESULT( mkstemp(tmpfile_template), &fd, fd!=-1&&errno==0 );

    /*generic fs test pread/pwrite*/
    test_zrt_issue_81(tmpfile_template);

    /*devices fs test dup functions*/
    test_zrt_issue_78(READ_WRITE_CHANNEL);
    test_zrt_issue_78(tmpfile_template);
}

void test_zrt_issue_81(const char* filename){
    const char gap[] = "xxx";
    char buf[PATH_MAX];
    int fd, ret;
    
    TEST_OPERATION_RESULT( open(filename, O_RDWR), &fd, fd!=-1&&errno==0 );

    /*test pwrite*/
    TEST_OPERATION_RESULT( pwrite(fd, 
				  filename, 
				  strlen(filename),
				  strlen(gap)), &ret, ret==strlen(filename)&&errno==0 );
    TEST_OPERATION_RESULT( pwrite(fd, 
				  gap, 
				  strlen(gap),
				  0), &ret, ret==strlen(gap)&&errno==0 );

    /*test pread*/
    TEST_OPERATION_RESULT( pread(fd, 
				 buf, 
				 strlen(filename),
				 strlen(gap)), &ret, ret==strlen(filename)&&errno==0 );
    TEST_OPERATION_RESULT( strncmp(buf, filename, strlen(filename)), &ret, ret==0 );
    TEST_OPERATION_RESULT( pread(fd, 
				 buf, 
				 strlen(gap),
				 0), &ret, ret==strlen(gap)&&errno==0 );
    TEST_OPERATION_RESULT( strncmp(buf, gap, strlen(gap)), &ret, ret==0 );
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0 );
}

void test_zrt_issue_78(const char* filename){
    int dupfd, fd, ret;
    char buf[PATH_MAX];
    const int test_offset = 100;
    
    /*dup tests*/
    TEST_OPERATION_RESULT( open(filename, O_RDWR), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( lseek(fd, test_offset, SEEK_SET), &ret, ret==test_offset&&errno==0 );
    TEST_OPERATION_RESULT( write(fd, filename, strlen(filename) ), &ret, ret==strlen(filename)&&errno==0 );
    TEST_OPERATION_RESULT( lseek(fd, test_offset, SEEK_SET), &ret, ret==test_offset&&errno==0 );

    /*get new handle and close old*/
    TEST_OPERATION_RESULT( dup(fd), &dupfd, dupfd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0 );

    TEST_OPERATION_RESULT( lseek(dupfd, 0, SEEK_CUR), &ret, ret==test_offset&&errno==0 );
    TEST_OPERATION_RESULT( read(dupfd, buf, strlen(filename)), &ret, ret==strlen(filename)&&errno==0 );
    
    TEST_OPERATION_RESULT( close(dupfd), &ret, ret==0&&errno==0 );
}

