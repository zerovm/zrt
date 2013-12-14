/*
 * Description
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "macro_tests.h"

#define TEST_FILE2 (TEST_FILE "2")

int main(int argc, char**argv){
    CREATE_FILE(TEST_FILE, DATA_FOR_FILE, DATASIZE_FOR_FILE);

    /*unlink*/
    REMOVE_EXISTING_FILEPATH(TEST_FILE);
    
    CREATE_FILE(TEST_FILE2, DATA_FOR_FILE, DATASIZE_FOR_FILE);

    int ret;
    TEST_OPERATION_RESULT(
			  link(TEST_FILE2, TEST_FILE),
			  &ret, ret!=-1);
    /*now we have 2 hardlinks for a single file*/
    /*compare files*/
    int fd1, fd2;
    char *data1, *data2;
    off_t filesize1, filesize2;
    MMAP_READONLY_SHARED_FILE(TEST_FILE, 0, &fd1, data1);
    MMAP_READONLY_SHARED_FILE(TEST_FILE2, 0, &fd2, data2);

    GET_FILE_SIZE(TEST_FILE, &filesize1);
    GET_FILE_SIZE(TEST_FILE2, &filesize2);
    
    TEST_OPERATION_RESULT( (filesize1==filesize2),
			   &ret, ret==1);

    CMP_MEM_DATA(data1, data2, filesize1);

    //check stat data
    struct stat st1;
    struct stat st2;
    TEST_OPERATION_RESULT(
			  stat(TEST_FILE, &st1),
			  &ret, ret==0);
    TEST_OPERATION_RESULT(
			  stat(TEST_FILE2, &st2),
			  &ret, ret==0);
    TEST_OPERATION_RESULT( 
			  st1.st_nlink==st2.st_nlink,
			  &ret, ret!=0);
    TEST_OPERATION_RESULT( 
			  st1.st_ino==st2.st_ino,
			  &ret, ret!=0);

    MUNMAP_FILE(data1, filesize1);
    MUNMAP_FILE(data2, filesize2);

    CLOSE_FILE(fd1);
    CLOSE_FILE(fd2);

    return 0;
}

