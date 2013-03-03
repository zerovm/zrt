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

#include "channels/test_channels.h"

int main(int argc, char **argv)
{
    char resolved_path[PATH_MAX];
    char* res = realpath( NULL, resolved_path);
    assert(res==NULL&&errno==EINVAL);

    res = realpath( "", resolved_path);
    assert(res==NULL&&errno==ENOENT);

    CREATE_FILE(TEST_FILE+1, "some data", sizeof("some data"));
    res = realpath( TEST_FILE, resolved_path);
    assert(res!=NULL);

    int64_t size = sizeof(TEST_FILE);
    CMP_MEM_DATA(res, TEST_FILE, size);
    
    if ( res ){
	fprintf(stderr, "realpath =%s, %s\n", res, resolved_path);
    }
    else{
	fprintf(stderr, "realpath failed, errno=%d\n", errno );
    }

    return !res;
}


