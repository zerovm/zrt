/*
 * poll test
 *
 * Copyright (c) 2014, LiteStack, Inc.
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


#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#ifdef __ZRT__
# define ZVM_TEST
#endif //__ZRT__

#ifdef ZVM_TEST
# include "macro_tests.h"
#endif 

#define READ_FILE  1
#define WRITE_FILE 2
#define TEST_DATA "hello"

struct pollfd s_ufd;
char s_buffer[10];

static void *thread_func(void *vptr_args){
    int bytes=0;
    int poll_ret=0;
    if ( vptr_args == (void*)READ_FILE ){
	printf("poll\n");
	if ( (poll_ret=poll(&s_ufd, 1, 1)) > 0 && s_ufd.revents & POLLIN ){
	    int s= lseek(s_ufd.fd, 0, SEEK_SET);
#ifdef ZVM_TEST
	(void)s;
#else
	printf("seek %d\n", s);
#endif //ZVM_TEST
	    bytes = read(s_ufd.fd, s_buffer, sizeof(s_buffer) );
	    printf("readed %s\n", s_buffer);
	}
	else{
	    fprintf(stderr, "pool ret=%d\n", poll_ret);
	}
    }
    else if ( vptr_args == (void*)WRITE_FILE ){
	printf("write\n");
	bytes = write(s_ufd.fd, TEST_DATA, strlen(TEST_DATA) );
    }
    return NULL;
}

int main(void){
    /*create tmp file*/
    FILE* tmp_file = tmpfile();
#ifdef ZVM_TEST
    int ret;
    TEST_OPERATION_RESULT(tmp_file != NULL, &ret, ret==1);
#endif

    /*init polling descriptor*/
    s_ufd.fd = fileno(tmp_file);
    s_ufd.events = POLLIN; /*wait for read|write event*/

    pthread_t thread_r;
    pthread_t thread_w;
    if (pthread_create(&thread_r, NULL, thread_func, (void*)READ_FILE ) != 0) {
	return EXIT_FAILURE;
    }
    if (pthread_create(&thread_w, NULL, thread_func, (void*)WRITE_FILE ) != 0) {
	return EXIT_FAILURE;
    }

    if (pthread_join(thread_r, NULL) != 0 || pthread_join(thread_w, NULL) != 0) {
	return EXIT_FAILURE;
    }

#ifdef ZVM_TEST
    TEST_OPERATION_RESULT(!strcmp(TEST_DATA, s_buffer), &ret, ret==1);
#else
    if ( strcmp(TEST_DATA, s_buffer) )
	fprintf(stderr, "error: buffer data mismatch\n");
#endif
    fclose(tmp_file);
    return EXIT_SUCCESS;
}


