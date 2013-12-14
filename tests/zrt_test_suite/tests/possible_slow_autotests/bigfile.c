/*
 * LFS test
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

#define HAVE_ZRT
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h> //getenv
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include "macro_tests.h"

int main(int argc, char **argv)
{
    int ret;
    int fd = open64( "/dev/writeonly", O_WRONLY );
    assert(fd >= 0);

    off64_t seekpos = 1024*1024*1024;
    seekpos *= 5; /*5 GB*/
    --seekpos;
    off64_t pos = lseek64( fd, seekpos, SEEK_SET);
    TEST_OPERATION_RESULT(
			  pos == seekpos,
			  &ret, ret!=0 );
    int bytes;
    TEST_OPERATION_RESULT(
			  write(fd, "1", 1 ),
			  &bytes, bytes==1);
    close(fd);
    return 0;
}
