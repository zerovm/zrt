/*
 * realpath using
 */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "channels/test_channels.h"

int main(int argc, char **argv)
{
    int ret;
    char buf[PATH_MAX];
    char* res = getcwd(buf, PATH_MAX);
    char* nullstr = NULL;

    TEST_OPERATION_RESULT(
			  rmdir(nullstr),
			  &ret, ret==-1&&errno==EINVAL );
    TEST_OPERATION_RESULT(
			  rmdir("nonexistent"),
			  &ret, ret==-1&&errno==ENOENT );
    CREATE_NON_EMPTY_DIR(DIR_NAME, DIR_FILE);
    TEST_OPERATION_RESULT(
			  rmdir(DIR_NAME),
			  &ret, ret==-1&&errno==ENOTEMPTY );

    #define FULL_DIR_FILE DIR_NAME "/" DIR_FILE
    TEST_OPERATION_RESULT(
			  rmdir(FULL_DIR_FILE),
			  &ret, ret==-1&&errno==ENOTDIR );

    CREATE_EMPTY_DIR(DIR_NAME2);
    TEST_OPERATION_RESULT(
			  rmdir(DIR_NAME2),
			  &ret, ret==0 );

    return !res;
}


