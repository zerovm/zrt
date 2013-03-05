#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "channels/test_channels.h"


int main(int argc, char**argv){
    int ret;
    TEST_OPERATION_RESULT(
			  euidaccess(TEST_FILE, F_OK),
			  &ret, ret==-1);

    CREATE_FILE(TEST_FILE, DATA_FOR_FILE, DATASIZE_FOR_FILE);
    
    TEST_OPERATION_RESULT(
			  euidaccess(TEST_FILE, F_OK),
			  &ret, ret==0);

    return 0;
}
