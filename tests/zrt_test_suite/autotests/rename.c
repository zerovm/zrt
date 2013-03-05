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
    char* nullstr = NULL;

    TEST_OPERATION_RESULT(
			  rename(nullstr, TEST_FILE),
			  &ret, ret==-1&&errno==EFAULT );

    TEST_OPERATION_RESULT(
			  rename(TEST_FILE "2", TEST_FILE),
			  &ret, ret==-1&&errno==ENOENT );

    CREATE_FILE(TEST_FILE, DATA_FOR_FILE, DATASIZE_FOR_FILE);

    TEST_OPERATION_RESULT(
			  rename(TEST_FILE, nullstr),
			  &ret, ret==-1&&errno==EFAULT );

    TEST_OPERATION_RESULT(
			  rename(TEST_FILE, TEST_FILE "2"),
			  &ret, ret==0 );

    CHECK_FILE_EXISTANCE(TEST_FILE "2");

    /*create file again on old path*/
    CREATE_FILE(TEST_FILE, DATA_FOR_FILE, DATASIZE_FOR_FILE);

    /*directory can't be renamed*/
    CREATE_NON_EMPTY_DIR(DIR_NAME,DIR_FILE);
    TEST_OPERATION_RESULT(
			  rename(DIR_NAME, DIR_NAME "2"),
			  &ret, ret==-1&&errno==EISDIR );

    /*move file into another directory, 
      oldpath is a file, newpath is a dir path*/
    TEST_OPERATION_RESULT(
			  rename(TEST_FILE "2", DIR_NAME ),
			  &ret, ret==0 );

    CHECK_FILE_EXISTANCE(DIR_NAME "" TEST_FILE "2");

    /*move file into another directory specifying full path*/
    #define FULL_DIR_FILE DIR_NAME "/" DIR_FILE
    TEST_OPERATION_RESULT(
			  rename(TEST_FILE, FULL_DIR_FILE),
			  &ret, ret==0 );

    CHECK_FILE_EXISTANCE(FULL_DIR_FILE);

    /*create file again on old path*/
    CREATE_FILE(TEST_FILE, DATA_FOR_FILE, DATASIZE_FOR_FILE);

    /*move into fake path, trying use file as directory name*/
    TEST_OPERATION_RESULT(
			  rename(TEST_FILE, FULL_DIR_FILE "/fakepath"),
			  &ret, ret==-1&&errno==ENOTDIR );
    return 0;
}


