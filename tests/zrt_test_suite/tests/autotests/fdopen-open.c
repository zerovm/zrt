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

#include "macro_tests.h"
#include "handle_allocator.h" //MAX_HANDLES_COUNT

void test_zrt_issue_79();
void test_open_limits();

int main(int argc, char **argv)
{
    int fd, ret;
    FILE *f, *f2;
    test_open_limits();
    TEST_OPERATION_RESULT( open("/newfile123", O_CREAT|O_RDWR), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( fdopen(fd, "r"), &f, f!=NULL&&errno==0);
    TEST_OPERATION_RESULT( fdopen(fd, "w"), &f2, f2!=NULL&&errno==0);
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0);

    test_zrt_issue_79();
}


void test_open_limits(){
    int i, ret;
    int maxhandlescount = MAX_HANDLES_COUNT;
    maxhandlescount -= 3; /* dev/stdin, dev/stdout, dev/stderr */
    int handles[MAX_HANDLES_COUNT];

    for (i=0; i < maxhandlescount; i++){
	TEST_OPERATION_RESULT( open("/dev/stdin", O_RDONLY, 0), &ret, ret!=-1&&errno==0);
	fprintf(stderr, "fd=%d\n", ret );
	handles[i] = ret;
    }
    /*can't open more files*/
    TEST_OPERATION_RESULT( open("/dev/stdin", O_RDONLY), &ret, ret==-1&&errno==ENFILE);
    TEST_OPERATION_RESULT( open("/dev", O_RDONLY|O_DIRECTORY), &ret, ret==-1&&errno==ENFILE);
    FILE *f = tmpfile();
    TEST_OPERATION_RESULT( f==NULL, &ret, ret==1&&errno==ENFILE);
    TEST_OPERATION_RESULT( open("/tmp", O_RDONLY|O_DIRECTORY), &ret, ret==-1&&errno==ENFILE);

    for (i=0; i < maxhandlescount; i++){
	TEST_OPERATION_RESULT( close(handles[i]), &ret, ret==0&&errno==0);
    }
}

void test_zrt_issue_79(){
    const char fdev[] = "/dev/null";
    const char fname[] = "/newfile124";
    char buf[PATH_MAX];
    int fdr, fdw, ret;
    int fd1, fd2;

    /*Devices fs test*/
    TEST_OPERATION_RESULT( open(fdev, O_WRONLY), &fd1, fd1!=-1&&errno==0 );
    TEST_OPERATION_RESULT( open(fdev, O_WRONLY), &fd2, fd2!=-1&&errno==0 );
    TEST_OPERATION_RESULT( write(fd1, fname, strlen(fname) ), &ret, ret!=-1&&ret==strlen(fname)&&errno==0 );
    TEST_OPERATION_RESULT( write(fd2, fname, strlen(fname) ), &ret, ret!=-1&&ret==strlen(fname)&&errno==0 );
    TEST_OPERATION_RESULT( close(fd1), &ret, ret==0&&errno==0);
    TEST_OPERATION_RESULT( close(fd2), &ret, ret==0&&errno==0);

    /*InMemory fs test*/
    TEST_OPERATION_RESULT( open(fname, O_CREAT|O_WRONLY), &fdw, fdw!=-1&&errno==0 );
    TEST_OPERATION_RESULT( open(fname, O_CREAT|O_RDONLY), &fdr, fdr!=-1&&errno==0 );

    TEST_OPERATION_RESULT( write(fdw, fname, strlen(fname) ), &ret, ret!=-1&&ret==strlen(fname)&&errno==0 );
    TEST_OPERATION_RESULT( read(fdr, buf, PATH_MAX ), &ret, ret!=-1&&ret==strlen(fname)&&errno==0 );

    TEST_OPERATION_RESULT( close(fdw), &ret, ret==0&&errno==0);
    TEST_OPERATION_RESULT( close(fdr), &ret, ret==0&&errno==0);
}
