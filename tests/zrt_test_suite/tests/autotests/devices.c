/*
 * testing sequential channels
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#include "macro_tests.h"

#define SEQUENTIAL_CHANNEL S_IFIFO

void test_readonly_channel(const char* name);
void test_writeonly_channel(const char* name);

void test_emu_devices_for_reading();
void test_emu_devices_for_writing();

#define BUFFER_LEN 0x1000
char s_buffer[BUFFER_LEN];

int main(int argc, char**argv){
    test_emu_devices_for_reading();
    test_readonly_channel(CHANNEL_NAME_READONLY);
    test_writeonly_channel(CHANNEL_NAME_WRITEONLY);
    return 0;
}

void test_emu_devices_for_writing(){
    int ret;
    char* emu_channel_name;
    char buf[10000];
    int fd;

    /*test emulated channels*/
    emu_channel_name = "/dev/zero";
    TEST_OPERATION_RESULT(
			  open(emu_channel_name, O_RDWR), 
			  &ret, ret != -1 );
    fd = ret;
    TEST_OPERATION_RESULT(
			  write(fd, buf, sizeof(buf)), 
			  &ret, ret == sizeof(buf) );
    TEST_OPERATION_RESULT(
			  close(fd), 
			  &ret, ret != -1 );

    emu_channel_name = "/dev/null";
    TEST_OPERATION_RESULT(
			  open(emu_channel_name, O_RDWR), 
			  &ret, ret != -1 );
    fd = ret;
    TEST_OPERATION_RESULT(
			  write(fd, buf, sizeof(buf)), 
			  &ret, ret == sizeof(buf) );
    TEST_OPERATION_RESULT(
			  close(fd), 
			  &ret, ret != -1 );

    emu_channel_name = "/dev/full";
    TEST_OPERATION_RESULT(
			  open(emu_channel_name, O_RDWR), 
			  &ret, ret != -1 );
    fd = ret;
    TEST_OPERATION_RESULT(
			  write(fd, buf, sizeof(buf)), 
			  &ret, ret==-1&&errno==ENOSPC );
    TEST_OPERATION_RESULT(
			  close(fd), 
			  &ret, ret != -1 );

    emu_channel_name = "/dev/random";
    TEST_OPERATION_RESULT(
			  open(emu_channel_name, O_RDWR), 
			  &ret, ret != -1 );
    fd = ret;
    TEST_OPERATION_RESULT(
			  write(fd, buf, sizeof(buf)), 
			  &ret, ret == sizeof(buf) );
    TEST_OPERATION_RESULT(
			  close(fd), 
			  &ret, ret != -1 );

    emu_channel_name = "/dev/urandom";
    TEST_OPERATION_RESULT(
			  open(emu_channel_name, O_RDONLY), 
			  &ret, ret != -1 );
    fd = ret;
    TEST_OPERATION_RESULT(
			  write(fd, buf, sizeof(buf)), 
			  &ret, ret == sizeof(buf) );
    TEST_OPERATION_RESULT(
			  close(fd), 
			  &ret, ret != -1 );

}


void test_emu_devices_for_reading(){
    int ret;
    char* emu_channel_name;
    char buf[10000];
    int fd;

    /*test emulated channels*/
    emu_channel_name = "/dev/zero";
    TEST_OPERATION_RESULT(
			  open(emu_channel_name, O_RDWR), 
			  &ret, ret != -1 );
    fd = ret;
    TEST_OPERATION_RESULT(
			  read(fd, buf, sizeof(buf)), 
			  &ret, ret == sizeof(buf) );
    TEST_OPERATION_RESULT(
			  close(fd), 
			  &ret, ret != -1 );

    emu_channel_name = "/dev/null";
    TEST_OPERATION_RESULT(
			  open(emu_channel_name, O_RDWR), 
			  &ret, ret != -1 );
    fd = ret;
    TEST_OPERATION_RESULT(
			  read(fd, buf, sizeof(buf)), 
			  &ret, ret == 0 );
    TEST_OPERATION_RESULT(
			  close(fd), 
			  &ret, ret != -1 );

    emu_channel_name = "/dev/full";
    TEST_OPERATION_RESULT(
			  open(emu_channel_name, O_RDWR), 
			  &ret, ret != -1 );
    fd = ret;
    TEST_OPERATION_RESULT(
			  read(fd, buf, sizeof(buf)), 
			  &ret, ret == sizeof(buf) );
    TEST_OPERATION_RESULT(
			  close(fd), 
			  &ret, ret != -1 );

    emu_channel_name = "/dev/random";
    TEST_OPERATION_RESULT(
			  open(emu_channel_name, O_RDWR), 
			  &ret, ret != -1 );
    fd = ret;
    TEST_OPERATION_RESULT(
			  read(fd, buf, sizeof(buf)), 
			  &ret, ret == sizeof(buf) );
    TEST_OPERATION_RESULT(
			  close(fd), 
			  &ret, ret != -1 );

    emu_channel_name = "/dev/urandom";
    TEST_OPERATION_RESULT(
			  open(emu_channel_name, O_RDONLY), 
			  &ret, ret != -1 );
    fd = ret;
    TEST_OPERATION_RESULT(
			  read(fd, buf, sizeof(buf)), 
			  &ret, ret == sizeof(buf) );
    TEST_OPERATION_RESULT(
			  close(fd), 
			  &ret, ret != -1 );
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
