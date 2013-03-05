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
    char* res;

    TEST_OPERATION_RESULT( realpath( NULL, resolved_path),
			   &res, res==NULL&&errno==EINVAL );

    TEST_OPERATION_RESULT( realpath( "", resolved_path),
			   &res, res==NULL&&errno==ENOENT );

    /*create file not using absolute name, note filenam eis TEST_FILE+1*/
    CREATE_FILE(TEST_FILE+1, "some data", sizeof("some data"));

    TEST_OPERATION_RESULT( realpath( TEST_FILE, resolved_path),
			   &res, res!=NULL );

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


