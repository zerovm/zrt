/*
 * truncate functions testing
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


#include <errno.h>
#include <error.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "macro_tests.h"

//#include "tst-truncate.h"

//functions
#define FTRUNCATE ftruncate
#define TRUNCATE  truncate
#define STRINGIFY(s) #s

//values
#define FILENAME "/truncatefile"
#define EXIT_FAILURE -1

void test_issue_69();

int tell(int fd)
{
    return lseek(fd, 0, SEEK_CUR);
}

int
main (int argc, char **argv)
{
    int fd = open(FILENAME, O_RDWR|O_CREAT);
    if (fd < 0)
	error (EXIT_FAILURE, errno, "during open");

    char *name = FILENAME;
    struct stat st;
    char buf[1000];

    memset (buf, '\0', sizeof (buf));

    if (write (fd, buf, sizeof (buf)) != sizeof (buf))
	error (EXIT_FAILURE, errno, "during write");

    if (fstat (fd, &st) < 0 || st.st_size != sizeof (buf))
	error (EXIT_FAILURE, 0, "initial size wrong");

    if (FTRUNCATE (fd, 800) < 0)
	error (EXIT_FAILURE, errno, "size reduction with %s failed",
	       STRINGIFY (FTRUNCATE));

    if (fstat (fd, &st) < 0 || st.st_size != 800)
	error (EXIT_FAILURE, 0, "size after reduction with %s incorrect",
	       STRINGIFY (FTRUNCATE));

    /* The following test covers more than POSIX.  POSIX does not require
       that ftruncate() can increase the file size.  But we are testing
       Unix systems.  */
    if (FTRUNCATE (fd, 1200) < 0)
	error (EXIT_FAILURE, errno, "size increase with %s failed",
	       STRINGIFY (FTRUNCATE));

    if (fstat (fd, &st) < 0 || st.st_size != 1200)
	error (EXIT_FAILURE, 0, "size after increase with %s incorrect",
	       STRINGIFY (FTRUNCATE));


    if (TRUNCATE (name, 800) < 0)
	error (EXIT_FAILURE, errno, "size reduction with %s failed",
	       STRINGIFY (TRUNCATE));

    if (fstat (fd, &st) < 0 || st.st_size != 800)
	error (EXIT_FAILURE, 0, "size after reduction with %s incorrect",
	       STRINGIFY (TRUNCATE));

    /* The following test covers more than POSIX.  POSIX does not require
       that truncate() can increase the file size.  But we are testing
       Unix systems.  */
    if (TRUNCATE (name, 1200) < 0)
	error (EXIT_FAILURE, errno, "size increase with %s failed",
	       STRINGIFY (TRUNCATE));

    if (fstat (fd, &st) < 0 || st.st_size != 1200)
	error (EXIT_FAILURE, 0, "size after increase with %s incorrect",
	       STRINGIFY (TRUNCATE));

    close (fd);
    unlink (name);

    test_issue_69();
    return 0;
}

void test_issue_69(){
    int ret;
    int fd;
    char buffer[1024];
    char filename[] = "@tmp_1_test";

    TEST_OPERATION_RESULT( open(filename, O_WRONLY | O_CREAT), &fd, fd!=-1&&errno==0);
    TEST_OPERATION_RESULT( write(fd, "12345678901", 11), &ret, ret==11&&errno==0);
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0 );

    TEST_OPERATION_RESULT( open(filename, O_RDONLY), &fd, fd!=-1&&errno==0);
    TEST_OPERATION_RESULT( read(fd, buffer, 5), &ret, ret!=0&&errno==0 );
    TEST_OPERATION_RESULT( lseek(fd, 0, SEEK_CUR), &ret, ret==5&&errno==0);
    TEST_OPERATION_RESULT( ftruncate(fd, 0), &ret, ret==-1&&errno==EINVAL);
    TEST_OPERATION_RESULT( lseek(fd, 0, SEEK_CUR), &ret, ret==5&&errno==0);
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0);

    TEST_OPERATION_RESULT( open(filename, O_CREAT | O_RDWR), &fd, fd!=-1&&errno==0 );
    TEST_OPERATION_RESULT( write(fd, "12345678901", 11), &ret, ret==11&&errno==0);
    TEST_OPERATION_RESULT( lseek(fd, -1, SEEK_END), &ret, ret==10&&errno==0);
    TEST_OPERATION_RESULT( ftruncate(fd, 9), &ret, ret==0&&errno==0);
    TEST_OPERATION_RESULT( tell(fd), &ret, ret==10&&errno==0);
    TEST_OPERATION_RESULT( close(fd), &ret, ret==0&&errno==0);
}
