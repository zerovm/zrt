/*
 * realpath using
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include "helpers/utils.h"

#include "channels/test_channels.h"

static char* res;
static char resolved_path[PATH_MAX];

void test_relative_path(const char* relative_path, const char* abs_path){
    int ret;

   /*create file not using absolute name, note filenam is TEST_FILE+1*/
    CREATE_FILE(relative_path, "some data", sizeof("some data"));

    TEST_OPERATION_RESULT( realpath( relative_path, resolved_path),
			   &res, res!=NULL );

    /*realpath & abspath must be equal*/
    TEST_OPERATION_RESULT(strcmp(res, abs_path), &ret, ret==0);
    /*check file existance using abs_path*/
    CHECK_PATH_EXISTANCE(abs_path);
}

int main(int argc, char **argv)
{
    /*test bad cases*/
    TEST_OPERATION_RESULT( realpath( NULL, resolved_path),
			   &res, res==NULL&&errno==EINVAL );

    TEST_OPERATION_RESULT( realpath( "", resolved_path),
			   &res, res==NULL&&errno==ENOENT );

    CREATE_EMPTY_DIR("/dev/mount/../../dir");
    /*test fs in memory*/
    test_relative_path("/dir/../foo1", "/foo1");
    test_relative_path("foo", "/foo");
    test_relative_path("/dir/../foo1", "/foo1");

    struct stat buf;
    int res = stat("/dir/../foo1", &buf);
    assert(res==0);

    /*test channels fs: /dev/nvram*/
    CHECK_PATH_EXISTANCE("/dev/mount/../nvram");

    return 0;
}


