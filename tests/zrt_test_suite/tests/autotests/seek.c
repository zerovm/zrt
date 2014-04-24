/*
 * seek demo tests
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


#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>


#include "macro_tests.h"


off_t tell(int fd)
{
    return lseek(fd, 0, SEEK_CUR);
}
void zrt_test_issue71()
{
    char filename[] = "@tmp_1_test";
    int fd, ret;

    TEST_OPERATION_RESULT( open(filename, O_CREAT | O_RDWR), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( write(fd, "12345678901", 11), &ret, ret==11&&errno==0 );
    TEST_OPERATION_RESULT( lseek(fd, -5, SEEK_END), &ret, ret==6&&errno==0);
    TEST_OPERATION_RESULT( tell(fd), &ret, ret==6&&errno==0);
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0 );
    REMOVE_EXISTING_FILEPATH(filename);
}

void zrt_test_issue72(){
    char filename[] = "@tmp_1_test";
    off_t LARGE = 2147483648;
    int fd, ret;

    TEST_OPERATION_RESULT( open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666), &fd, fd!=-1&&errno==0 );
    off_t s = lseek(fd, LARGE, SEEK_SET);
    fprintf(stderr, "lseek=%lld\n", s);
    TEST_OPERATION_RESULT( s==LARGE&&errno==0, &ret, ret==1 );
    s = tell(fd);
    fprintf(stderr, "tell=%lld\n", s);
    TEST_OPERATION_RESULT( s==LARGE&&errno==0, &ret, ret==1);
    TEST_OPERATION_RESULT( write(fd, "12345678901", 11), &ret, ret==11&&errno==0);
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0 );
    REMOVE_EXISTING_FILEPATH(filename);
}

void zrt_test_issue_102_103(){
    const char* append_only_file = "/dev/read-write";
    int fd, ret;
    TEST_OPERATION_RESULT( open(append_only_file, O_RDWR|O_CREAT|O_APPEND, 0666), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( write(fd, append_only_file, strlen(append_only_file)), &ret, ret==strlen(append_only_file)&&errno==0 );
    TEST_OPERATION_RESULT( lseek(fd, 0, SEEK_END), &ret, ret==strlen(append_only_file)&&errno==0 );
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0 );

    TEST_OPERATION_RESULT( open(append_only_file, O_RDWR, 0666), &fd, fd==-1&&errno==EPERM );
    TEST_OPERATION_RESULT( open(append_only_file, O_RDWR|O_APPEND|O_TRUNC, 0666), &fd, fd==-1&&errno==EPERM );

    TEST_OPERATION_RESULT( open(append_only_file, O_RDWR|O_APPEND, 0666), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( lseek(fd, 0, SEEK_END), &ret, ret==strlen(append_only_file)&&errno==0 );
    TEST_OPERATION_RESULT( write(fd, append_only_file, strlen(append_only_file)), &ret, ret==strlen(append_only_file)&&errno==0 );
    TEST_OPERATION_RESULT( lseek(fd, 0, SEEK_END), &ret, ret==(2*strlen(append_only_file))&&errno==0 );
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0 );
}

int main(int argc, char **argv)
{
    int ret;
    FILE *f;
    const char* fname1 = "/seeker.data";
    const char* fname2 = "/seeker2.data";
    const char *str = "hip hop ololey";
    const char *str2 = "opa-opa-opa-pa";
    const char *str3 = "hip hop opa-opa-opa-pa";

    zrt_test_issue71();
    zrt_test_issue72();

    zrt_test_issue_102_103();

    f = fopen( fname1, "w" );
    TEST_OPERATION_RESULT(  f>0, &ret, ret==1);
    TEST_OPERATION_RESULT( fseek(f, -1, SEEK_SET), &ret, ret==-1&&errno==EINVAL);
    TEST_OPERATION_RESULT( fwrite(str, 1, strlen(str), f), &ret, ret==strlen(str)&&errno==0);
    TEST_OPERATION_RESULT( fseek( f, 0, SEEK_CUR ), &ret, ret==0&&errno==0);
    TEST_OPERATION_RESULT( ftell(f) == strlen(str), &ret, ret==1);
    TEST_OPERATION_RESULT( fseek( f, -6, SEEK_CUR ), &ret, ret==0);
    TEST_OPERATION_RESULT( ftell(f) == strlen(str)-6, &ret, ret==1);
    TEST_OPERATION_RESULT( fwrite(str2, 1, strlen(str2), f ), &ret, ret==strlen(str2));
    //fseek( f, 0, SEEK_END ); /*zrt known issue 5.1: SEEK_END replace by NACL on SEEK_SET*/
    fprintf(stderr, "file pos=%ld, expected pos=%u\n", ftell(f), strlen(str3) );
    TEST_OPERATION_RESULT( ftell(f) == strlen(str3), &ret, ret==1);
    TEST_OPERATION_RESULT( fclose(f), &ret, ret==0 );

    int fd = open( fname2, O_CREAT | O_WRONLY );
    assert( fd >= 0 );
    write(fd, str, strlen(str));
    off_t off;
    off = lseek( fd, 0, SEEK_CUR );
    assert( off == strlen(str) );

    off = lseek( fd, -6, SEEK_CUR );
    assert( off == strlen(str)-6 );

    write(fd, str2, strlen(str2) );
    off = lseek( fd, 0, SEEK_END );
    fprintf(stderr, "file pos=%lld, expected pos=%u\n", off, strlen(str3) );
    assert( off == strlen(str3) );
    close(fd);
    return 0;
}


