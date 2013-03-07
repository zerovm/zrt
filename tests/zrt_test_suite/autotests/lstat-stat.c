/* lstat, stat tests
*/
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
			  &ret, ret==0&&st.st_nlink==1);

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
			  &ret, ret==0&&st.st_nlink==1&&S_ISREG(st.st_mode));

    CREATE_EMPTY_DIR(DIR_NAME);
    TEST_OPERATION_RESULT(
			  stat(DIR_NAME, &st),
			  &ret, ret==0&&st.st_nlink==2&&S_ISDIR(st.st_mode));

    CREATE_EMPTY_DIR(DIR_NAME "/" DIR_NAME2);
    TEST_OPERATION_RESULT(
			  stat(DIR_NAME, &st),
			  &ret, ret==0&&st.st_nlink==3&&S_ISDIR(st.st_mode)!=0);

    TEST_OPERATION_RESULT(
			  rmdir(DIR_NAME "/" DIR_NAME2),
			  &ret, ret==0 );
    TEST_OPERATION_RESULT(
			  stat(DIR_NAME, &st),
			  &ret, ret==0&&st.st_nlink==2);

    REMOVE_EXISTING_FILEPATH(TEST_FILE);

    return 0;
}
