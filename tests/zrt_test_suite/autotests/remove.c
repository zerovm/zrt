/*
 * remove test
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
    struct stat st;
    char* nullstr = NULL;

    TEST_OPERATION_RESULT(
			  remove(nullstr),
			  &ret, ret==-1&&errno==EFAULT );
    TEST_OPERATION_RESULT(
			  remove("nonexistent"),
			  &ret, ret==-1&&errno==ENOENT );
    CREATE_NON_EMPTY_DIR(DIR_NAME, DIR_FILE);
    TEST_OPERATION_RESULT(
			  remove(DIR_NAME),
			  &ret, ret==-1&&errno==ENOTEMPTY );
    TEST_OPERATION_RESULT(
			  remove(DIR_NAME "/" DIR_FILE),
			  &ret, ret==0 );
    TEST_OPERATION_RESULT(
			  remove(DIR_NAME),
			  &ret, ret==0 );
    TEST_OPERATION_RESULT(
			  stat(DIR_NAME, &st),
			  &ret, ret==-1&&errno==ENOENT );

    return 0;
}


