#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "channels/test_channels.h"

#define JOIN_STR(a,b) a b
#define FILENAME getenv("FPATH")

int main(int argc, char**argv){
    char path[PATH_MAX];

    CHECK_PATH_EXISTANCE("/warm");
    sprintf(path, "/warm/%s", FILENAME );
    CHECK_PATH_EXISTANCE( path );

    CHECK_PATH_NOT_EXIST("/bad");
    sprintf(path, "/bad/%s", FILENAME );
    CHECK_PATH_NOT_EXIST( path );

    //files injected twice here, need to check files validity
    CHECK_PATH_EXISTANCE("/ok");
    sprintf(path, "/ok/%s", FILENAME );
    CHECK_PATH_EXISTANCE( path );

    //check file contents if injecting one of file that already exists
    int sz1,sz2,ret;
    sprintf(path, "/warm/%s", FILENAME );
    GET_FILE_SIZE(path, &sz1);

    sprintf(path, "/ok/%s", FILENAME );
    GET_FILE_SIZE(path, &sz2);
    TEST_OPERATION_RESULT( sz1==sz2,
    			   &ret, ret!=0);

    CHECK_PATH_NOT_EXIST("/bad1");
    CHECK_PATH_NOT_EXIST("/bad2");
    return 0;
}

