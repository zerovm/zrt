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

    /*mapping && file mode tests*/
    TEST_OPERATION_RESULT(
			  stat("/dev/stdin", &st),
			  &ret, ret==0&&
			  ((st.st_mode&S_IWUSR)!=S_IWUSR)&&
			  ((st.st_mode&S_IRUSR)==S_IRUSR)&&
			  S_ISCHR(st.st_mode));
    TEST_OPERATION_RESULT(
			  stat("/dev/stdout", &st),
			  &ret, ret==0&&
			  ((st.st_mode&S_IWUSR)==S_IWUSR)&&
			  ((st.st_mode&S_IRUSR)!=S_IRUSR)&&
			  S_ISFIFO(st.st_mode));
    TEST_OPERATION_RESULT(
			  stat("/dev/stderr", &st),
			  &ret, ret==0&&
			  ((st.st_mode&S_IWUSR)==S_IWUSR)&&
			  ((st.st_mode&S_IRUSR)!=S_IRUSR)&&
			  S_ISREG(st.st_mode));
    TEST_OPERATION_RESULT(
			  stat("/dev/read-write", &st),
			  &ret, ret==0&&
			  ((st.st_mode&S_IWUSR)==S_IWUSR)&&
			  ((st.st_mode&S_IRUSR)==S_IRUSR)&&
			  S_ISBLK(st.st_mode));

    return 0;
}
