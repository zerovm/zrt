#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "channels/test_channels.h"

#define JOIN_STR(a,b) a b

#define FILE_IN_TAR "/tests/zrt_test_suite/testfile.1234"

int main(int argc, char**argv){
    CHECK_PATH_EXISTANCE("/warm");
    CHECK_PATH_EXISTANCE( JOIN_STR("/warm", FILE_IN_TAR) );

    CHECK_PATH_NOT_EXIST("/bad");
    CHECK_PATH_NOT_EXIST( JOIN_STR("/bad", FILE_IN_TAR) );

    //files injected twice here, need to check files validity
    CHECK_PATH_EXISTANCE("/ok");
    CHECK_PATH_EXISTANCE( JOIN_STR("/ok", FILE_IN_TAR) );

    //check file contents if injecting one of file that already exists
    int sz1,sz2,ret;
    GET_FILE_SIZE(JOIN_STR("/warm", FILE_IN_TAR), &sz1);
    GET_FILE_SIZE(JOIN_STR("/ok", FILE_IN_TAR), &sz2);
    TEST_OPERATION_RESULT( sz1==sz2,
    			   &ret, ret!=0);

    CHECK_PATH_NOT_EXIST("/bad1");
    CHECK_PATH_NOT_EXIST("/bad2");
    return 0;
}

