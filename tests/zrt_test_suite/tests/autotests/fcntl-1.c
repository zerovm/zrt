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


#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include "macro_tests.h"

int main(int argc, char **argv)
{
    int ret;
    int fd;
    struct flock lock, savelock;
    char buf[100];
    int mode = O_CREAT|O_WRONLY;

    TEST_OPERATION_RESULT( fcntl(-1, F_SETLK), &ret, ret==-1 && errno==EBADF );
    
    TEST_OPERATION_RESULT( open("/testfile", mode), &fd, fd!=-1 && errno==0 );

    /*test file is not readable but avail for write*/
    TEST_OPERATION_RESULT( write(fd, "abcd", 4), &ret, ret==4&&errno==0 );
    TEST_OPERATION_RESULT( read(fd, buf, 4), &ret, ret==-1&&errno==EINVAL );

    /*check fcntl how it works with "Advisory locking"*/
    lock.l_type = F_WRLCK;
    /*it should not have any locks*/
    TEST_OPERATION_RESULT( fcntl(fd, F_GETLK, &lock), &ret, ret!=-1 && lock.l_type == F_UNLCK && errno==0 );

    /* Test for any lock on any part of file. */
    lock.l_type    = F_WRLCK;   
    lock.l_start   = 0;
    lock.l_whence  = SEEK_SET;
    lock.l_len     = 0;        
    savelock = lock;
    TEST_OPERATION_RESULT( fcntl(fd, F_SETLK, &savelock), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( fcntl(fd, F_GETLK, &lock), &ret, 
			   ret==0 && savelock.l_type==lock.l_type && errno==0 );

    TEST_OPERATION_RESULT( fcntl(fd, F_GETFL), &ret, ret==mode&&errno==0 );
    TEST_OPERATION_RESULT( fcntl(fd, F_SETFL, O_RDONLY), &ret, ret!=-1&&errno==0 );

    /*starting from now file is not writable and only can be read*/
    TEST_OPERATION_RESULT( write(fd, "abcd", 4), &ret, ret==-1&&errno==EINVAL );
    TEST_OPERATION_RESULT( lseek(fd, 0, SEEK_SET), &ret, ret==0&&errno==0 );
    TEST_OPERATION_RESULT( read(fd, buf, 4), &ret, ret==4&&errno==0&&!strcmp(buf, "abcd") );

    /*close file*/
    close(fd);
    return 0;
}


