/*
 * fdopen test, also this tests limit for max opened files.
 * Additionally it writes big amount of data into testing files to
 * locate memory leaks related to removing files that resides in
 * InMemory fs, doing it by manually analizing /dev/stderr,
 * /dev/alloc_report.
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
#include <malloc.h>
#include <limits.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include "macro_tests.h"
#include "handle_allocator.h" //MAX_HANDLES_COUNT

void test_issue_132();
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
    test_issue_132();
    exit(0); /*test exit 0 behavior*/
}

/*determine how many files can be opened additionally*/
static int can_open_max_files_count(){
#define PREOPEN_FILES_COUNT 10
    int handles[PREOPEN_FILES_COUNT];
    int max_handle=0;
    int i;
    for (i=0; i < PREOPEN_FILES_COUNT; i++){
        handles[i] = open("/dev/stdin", O_RDONLY, 0);
        /*if this fail further testings makes no sence*/
        assert(handles[i]!=-1);
    }
    for (i=0; i < PREOPEN_FILES_COUNT; i++){
        if (handles[i]>max_handle)
            max_handle = handles[i];
        close(handles[i]);
    }
    return MAX_HANDLES_COUNT - (1 + max_handle - PREOPEN_FILES_COUNT);
#undef PREOPEN_FILES_COUNT
}

void test_open_limits(){
    malloc_stats();
    fprintf(stdout, "sbrk(0)=%p before 1000files\n", sbrk(0));fflush(0);
    int i, ret;
    int count = 1024*1024;
    char* data = malloc(count);
    int maxhandlescount = can_open_max_files_count();
    FILE* handles[maxhandlescount];
    /*create files as many as possible, and fill it by big amount of
      data*/
    for (i=0; i < maxhandlescount; i++){
	handles[i] = tmpfile();
	TEST_OPERATION_RESULT( handles[i]!=NULL, &ret, ret==1);
	fprintf(stderr, "fd=%d\n", fileno(handles[i]) );
	TEST_OPERATION_RESULT( fwrite(data, 1, count, handles[i]), &ret, ret==count);
    }
    free(data);
    /*can't open more files*/
    TEST_OPERATION_RESULT( open("/dev/stdin", O_RDONLY), &ret, ret==-1&&errno==ENFILE);
    TEST_OPERATION_RESULT( open("/dev", O_RDONLY|O_DIRECTORY), &ret, ret==-1&&errno==ENFILE);
    FILE *f = tmpfile();
    TEST_OPERATION_RESULT( f==NULL, &ret, ret==1&&errno==ENFILE);
    TEST_OPERATION_RESULT( open("/tmp", O_RDONLY|O_DIRECTORY), &ret, ret==-1&&errno==ENFILE);

    fprintf(stdout, "sbrk(0)=%p 1000files not removed\n", sbrk(0));fflush(0);
    malloc_stats();
    /*this remove all opened files*/
    for (i=0; i < maxhandlescount; i++){
	TEST_OPERATION_RESULT( fclose(handles[i]), &ret, ret==0&&errno==0);
    }
    fprintf(stdout, "sbrk(0)=%p after 1000files\n", sbrk(0));fflush(0);
    malloc_stats();
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

void test_issue_132(){
    int fd, ret;
    struct stat st;
    const char *testpath;

    /*test pipe, mapped channel*/
    testpath = "/dev/stdin";
    fprintf(stderr, "test path %s\n", testpath);
    TEST_OPERATION_RESULT( stat(testpath, &st), &ret, ret!=-1&&errno==0 );
    TEST_OPERATION_RESULT( st.st_mode&S_IFMT&S_IFIFO, &ret, ret!=0 );

    TEST_OPERATION_RESULT( open(testpath, O_RDONLY), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( open(testpath, O_RDONLY | O_TRUNC), &fd, fd!=-1&&errno==0 );

    TEST_OPERATION_RESULT( open(testpath, O_WRONLY), &fd, fd==-1&&errno==EACCES );
    TEST_OPERATION_RESULT( open(testpath, O_RDWR), &fd, fd==-1&&errno==EACCES );

    /*test char, mapped channel*/
    testpath = "/dev/stdout";
    fprintf(stderr, "test path %s\n", testpath);
    TEST_OPERATION_RESULT( stat(testpath, &st), &ret, ret!=-1&&errno==0 );
    TEST_OPERATION_RESULT( st.st_mode&S_IFMT&S_IFCHR, &ret, ret!=0 );

    TEST_OPERATION_RESULT( open(testpath, O_RDONLY), &fd, fd==-1&&errno==EACCES );

    TEST_OPERATION_RESULT( open(testpath, O_WRONLY), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( open(testpath, O_WRONLY | O_TRUNC), &fd, fd!=-1&&errno==0 );

    TEST_OPERATION_RESULT( open(testpath, O_RDWR), &fd, fd==-1&&errno==EACCES );

    /*test file, mapped channel*/
    testpath = "/dev/file";
    fprintf(stderr, "test path %s\n", testpath);
    TEST_OPERATION_RESULT( stat(testpath, &st), &ret, ret!=-1&&errno==0 );
    TEST_OPERATION_RESULT( st.st_mode&S_IFMT&S_IFREG, &ret, ret!=0 );

    TEST_OPERATION_RESULT( open(testpath, O_RDONLY), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( open(testpath, O_RDONLY | O_TRUNC), &fd, fd==-1&&errno==EPERM );

    TEST_OPERATION_RESULT( open(testpath, O_WRONLY), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( open(testpath, O_WRONLY | O_TRUNC), &fd, fd==-1&&errno==EPERM );

    TEST_OPERATION_RESULT( open(testpath, O_RDWR), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( open(testpath, O_RDWR | O_TRUNC), &fd, fd==-1&&errno==EPERM );

    /*test block dev, mapped channel*/
    testpath = "/dev/blck";
    fprintf(stderr, "test path %s\n", testpath);
    TEST_OPERATION_RESULT( stat(testpath, &st), &ret, ret!=-1&&errno==0 );
    TEST_OPERATION_RESULT( st.st_mode&S_IFMT&S_IFBLK, &ret, ret!=0 );

    TEST_OPERATION_RESULT( open(testpath, O_RDONLY), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( open(testpath, O_RDONLY | O_TRUNC), &fd, fd==-1&&errno==EPERM );

    TEST_OPERATION_RESULT( open(testpath, O_WRONLY), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( open(testpath, O_WRONLY | O_TRUNC), &fd, fd==-1&&errno==EPERM );

    TEST_OPERATION_RESULT( open(testpath, O_RDWR), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( open(testpath, O_RDWR | O_TRUNC), &fd, fd==-1&&errno==EPERM );

    /*do not close channels at all, to keep it simple*/
}

