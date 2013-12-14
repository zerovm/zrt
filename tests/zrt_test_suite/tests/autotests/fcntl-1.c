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
#include <errno.h>
#include <assert.h>

int main(int argc, char **argv)
{
    int ret;
    ret=fcntl(-1, F_SETLK);
    assert( ret==-1 && errno==EBADF );
    
    /*create & open file in MemMount*/
    int mode = O_CREAT|O_WRONLY;
    int fd = open("/testfile", mode);
    assert(fd>0);
    /*check fcntl how it works with "Advisory locking"*/
    struct flock lock, savelock;
    
    /*test1*/
    lock.l_type = F_WRLCK;
    printf("F_GETLK l_type %d before, p=%p ", lock.l_type, &lock );fflush(0);
    ret = fcntl(fd, F_GETLK, &lock);  
    printf("l_type %d after, ret=%d\n",   lock.l_type, ret );fflush(0);
    assert(lock.l_type == F_UNLCK); /*it should not have any locks*/

    /*test2*/
    lock.l_type    = F_WRLCK;   /* Test for any lock on any part of file. */
    lock.l_start   = 0;
    lock.l_whence  = SEEK_SET;
    lock.l_len     = 0;        
    savelock = lock;
    fcntl(fd, F_SETLK, &savelock);  
    fcntl(fd, F_GETLK, &lock);  
    assert( savelock.l_type == lock.l_type );

    /*check fcntl how it works with "File status flags"*/
    //int res = fcntl(fd, F_GETFL);
    //printf("F_GETFL mode %d", res );
    //assert(res==mode);
    /*close file*/
    close(fd);
    return 0;
}


