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
    struct stat st;
    //lstat test
    TEST_OPERATION_RESULT(
			  lstat(TEST_FILE, &st),
			  &ret, (ret==-1&&errno==ENOENT)); 

    CREATE_FILE(TEST_FILE, DATA_FOR_FILE, DATASIZE_FOR_FILE);
    
    TEST_OPERATION_RESULT(
			  lstat(TEST_FILE, &st),
			  &ret, ret==0);

    REMOVE_EXISTING_FILEPATH(TEST_FILE);

    //////////////////////////////////////

    //stat test
    TEST_OPERATION_RESULT(
			  stat(TEST_FILE, &st),
			  &ret, (ret==-1&&errno==ENOENT));
    fprintf(stderr, "errno=%d\n", errno);fflush(0);

    CREATE_FILE(TEST_FILE, DATA_FOR_FILE, DATASIZE_FOR_FILE);
    
    TEST_OPERATION_RESULT(
			  stat(TEST_FILE, &st),
			  &ret, ret==0);

    REMOVE_EXISTING_FILEPATH(TEST_FILE);

    return 0;
}
