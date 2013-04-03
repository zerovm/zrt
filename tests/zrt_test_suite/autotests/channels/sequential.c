/*it's testing sequential channels only*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "test_channels.h"

#define SEQUENTIAL_CHANNEL S_IFIFO

void test_readonly_channel(const char* name);
void test_writeonly_channel(const char* name);

#define BUFFER_LEN 0x1000
char s_buffer[BUFFER_LEN];

int main(int argc, char**argv){
    test_readonly_channel(CHANNEL_NAME_READONLY);
    test_writeonly_channel(CHANNEL_NAME_WRITEONLY);
    return 0;
}


void test_readonly_channel(const char* name){
    int ret;
    int ret2;
    struct stat st;
    fprintf(stderr, "testing channel %s\n", name);
    /*test incorrect close operations*/
    TEST_OPERATION_RESULT(
			  close(-1), 
			  &ret, ret == -1 );
    TEST_OPERATION_RESULT(
			  close(10000), 
			  &ret, ret == -1 );
    /*test various of opens*/
    TEST_OPERATION_RESULT(
			  open(name, O_RDWR), 
			  &ret, ret == -1 );
    TEST_OPERATION_RESULT(
			  open(name, O_WRONLY), 
			  &ret, ret == -1 );
    TEST_OPERATION_RESULT(
			  open(name, O_RDONLY), 
			  &ret, ret != -1 );
    /*test i/o*/
    TEST_OPERATION_RESULT(
			  lseek(ret, 0, SEEK_CUR), 
			  &ret2, ret2 == 0 );

    TEST_OPERATION_RESULT(
			  read(ret, s_buffer, 10), 
			  &ret2, ret2 == 10 );

    TEST_OPERATION_RESULT(
			  lseek(ret, 0, SEEK_CUR), 
			  &ret2, ret2 == 10 );

    TEST_OPERATION_RESULT(
			  write(ret, s_buffer, 10), 
			  &ret2, ret2 == -1 );

    TEST_OPERATION_RESULT(
			  lseek(ret, 0, SEEK_CUR), 
			  &ret2, ret2 == 10 );

    TEST_OPERATION_RESULT(
			  lseek(ret, 10, SEEK_SET), 
			  &ret2, ret2 == -1 );
    TEST_OPERATION_RESULT(
			  fstat(ret, &st), 
			  &ret2, ret2 != -1 );
    TEST_OPERATION_RESULT(
			  st.st_mode&S_IFMT, 
			  &ret2, ret2 == SEQUENTIAL_CHANNEL );
    TEST_OPERATION_RESULT(
			  close(ret), 
			  &ret2, ret2 != -1 );
    TEST_OPERATION_RESULT(
			  close(ret), 
			  &ret, ret == -1 );
    TEST_OPERATION_RESULT(
			  stat(name, &st), 
			  &ret2, ret2 != -1 );
    TEST_OPERATION_RESULT(
			  st.st_mode&S_IFMT, 
			  &ret2, ret2 == SEQUENTIAL_CHANNEL );

}


void test_writeonly_channel(const char* name){
    int ret;
    int ret2;
    struct stat st;
    fprintf(stderr, "testing channel %s\n", name);
    /*test various of opens*/
    TEST_OPERATION_RESULT(
			  open(name, O_RDWR), 
			  &ret, ret == -1 );
    TEST_OPERATION_RESULT(
			  open(name, O_RDONLY), 
			  &ret, ret == -1 );
    TEST_OPERATION_RESULT(
			  open(name, O_WRONLY), 
			  &ret, ret != -1 );
    /*test i/o*/
    TEST_OPERATION_RESULT(
			  lseek(ret, 0, SEEK_CUR), 
			  &ret2, ret2 == 0 );

    TEST_OPERATION_RESULT(
			  read(ret, s_buffer, 10), 
			  &ret2, ret2 == -1 );

    TEST_OPERATION_RESULT(
			  lseek(ret, 0, SEEK_CUR), 
			  &ret2, ret2 == 0 );

    TEST_OPERATION_RESULT(
			  write(ret, s_buffer, 10), 
			  &ret2, ret2 == 10 );

    TEST_OPERATION_RESULT(
			  lseek(ret, 0, SEEK_CUR), 
			  &ret2, ret2 == 10 );

    TEST_OPERATION_RESULT(
			  lseek(ret, 10, SEEK_SET), 
			  &ret2, ret2 == -1 );
    TEST_OPERATION_RESULT(
			  fstat(ret, &st), 
			  &ret2, ret2 != -1 );
    TEST_OPERATION_RESULT(
			  st.st_mode&S_IFMT, 
			  &ret2, ret2 == SEQUENTIAL_CHANNEL );
    TEST_OPERATION_RESULT(
			  close(ret), 
			  &ret2, ret2 != -1 );
    TEST_OPERATION_RESULT(
			  close(ret), 
			  &ret, ret == -1 );
    TEST_OPERATION_RESULT(
			  stat(name, &st), 
			  &ret2, ret2 != -1 );
    TEST_OPERATION_RESULT(
			  st.st_mode&S_IFMT, 
			  &ret2, ret2 == SEQUENTIAL_CHANNEL );
}
