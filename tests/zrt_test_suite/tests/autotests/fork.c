/*
 * zfork test
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include <zrtapi.h>

#include "macro_tests.h"

#define FILENAME_WITH_DYNAMIC_CONTENTS getenv("FPATH")
#define MOUNT_CONTENTS "mount\n"
#define REMOUNT_CONTENTS "remount\n"

char* read_file_contents( const char* name, int* datalen ){
#define BUF_LEN_MAX 10000
    int fd = open( name, O_RDONLY );
    if ( fd != -1 ){
	char* contents = NULL;
	contents = calloc(BUF_LEN_MAX, sizeof(char));
	*datalen = read(fd, contents, BUF_LEN_MAX);
	close(fd);
	return contents;
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int res;
    int datalen, datalen1;
    char testpath[PATH_MAX];
    snprintf(testpath, sizeof(testpath), "/test/%s", FILENAME_WITH_DYNAMIC_CONTENTS );
    fprintf(stderr, "%s\n", testpath);

    /*Read files at mountpoint*/
    char* contents1 = read_file_contents( testpath, &datalen1 );
    CMP_MEM_DATA(MOUNT_CONTENTS, contents1, strlen(MOUNT_CONTENTS) );

    char* contents = read_file_contents( FILENAME_WITH_DYNAMIC_CONTENTS, &datalen );
    CMP_MEM_DATA(MOUNT_CONTENTS, contents, strlen(MOUNT_CONTENTS) );

    TEST_OPERATION_RESULT(strcmp("1", getenv("new")), &res, res==0);

    struct stat st;
    TEST_OPERATION_RESULT( lstat("/dev/read-write", &st), &res, res==0 );
    TEST_OPERATION_RESULT( (st.st_mode&S_IFCHR)==S_IFCHR, &res, res==1 );
    
    res = zfork();
    free(contents);
    free(contents1);
    /*After fork it is expected that this file remains unchanged*/
    contents1 = read_file_contents( testpath, &datalen1 );
    CMP_MEM_DATA(MOUNT_CONTENTS, contents1, strlen(MOUNT_CONTENTS) );

    /*After fork it is expected that this file is changed*/
    contents = read_file_contents( FILENAME_WITH_DYNAMIC_CONTENTS, &datalen );
    CMP_MEM_DATA(REMOUNT_CONTENTS, contents, strlen(REMOUNT_CONTENTS) );

    /*After fork env var changed*/
    TEST_OPERATION_RESULT(strcmp("2", getenv("new")), &res, res==0);

    /*After fork channels mapping is changed*/
    TEST_OPERATION_RESULT( lstat("/dev/read-write", &st), &res, res==0 );
    TEST_OPERATION_RESULT( (st.st_mode&S_IFREG)==S_IFREG, &res, res==1 );

    free(contents);
    free(contents1);
    return 0;
}
