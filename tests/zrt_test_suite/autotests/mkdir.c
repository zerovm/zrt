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
    char* nullstr = NULL;

    TEST_OPERATION_RESULT(
			  mkdir(nullstr, S_IRWXU),
			  &ret, ret==-1&&errno==EINVAL );
    TEST_OPERATION_RESULT(
			  mkdir("/" DIR_NAME, S_IRWXU),
			  &ret, ret==0 );
    TEST_OPERATION_RESULT(
			  mkdir(DIR_NAME, S_IRWXU),
			  &ret, ret==-1&&errno==EEXIST );

    CHECK_PATH_EXISTANCE("/" DIR_NAME);
    return 0;
}


