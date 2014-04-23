/*
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



#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h> //gettimeofday
#include <time.h> //clock_gettime
#include <sys/select.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <assert.h>
#include "macro_tests.h"

void prepare_time_test(struct timespec *ts){
    int ret;
    /*compare time before and after timeout, measure in nanoseconds*/
    TEST_OPERATION_RESULT( clock_gettime(CLOCK_REALTIME, ts), &ret, ret==0 );
}

void complete_nanosec_test(struct timespec *prepared_ts, struct timespec *timeout_ts){
    int ret;
    struct timespec new_ts;

    timeout_ts->tv_nsec += 1000; /*clock_gettime take additional time*/
    TEST_OPERATION_RESULT( clock_gettime(CLOCK_REALTIME, &new_ts), &ret, ret==0 );

    struct timeval prepared_tv;
    TIMESPEC_TO_TIMEVAL(&prepared_tv, prepared_ts);
    struct timeval new_tv;
    TIMESPEC_TO_TIMEVAL(&new_tv, &new_ts);
    struct timeval timeout_tv;
    TIMESPEC_TO_TIMEVAL(&timeout_tv, timeout_ts);
    struct timeval tv_inc;
    timeradd(&prepared_tv, &timeout_tv, &tv_inc);

    TEST_OPERATION_RESULT( timercmp(&tv_inc, &new_tv, == ), &ret, ret==1);
}

void test_clock(){
    struct timespec ts, expected_timeout_ts;
    
    /*save current time*/
    prepare_time_test(&ts);
    /*get tick count*/
    clock_t t = clock();
    (void)t;
    expected_timeout_ts.tv_sec = 0;
    expected_timeout_ts.tv_nsec = 1000; //1 microsec
    /*check the passed amount time as expected*/
    complete_nanosec_test(&ts, &expected_timeout_ts);
}


void test_sleep(int sec, int nsec, int expected_sec, int expected_nsec){
    int ret;
    struct timespec ts, expected_timeout_ts;
    
    /*save current time*/
    prepare_time_test(&ts);
    /*wait until timeout exceeded*/
    TEST_OPERATION_RESULT( sleep(sec), &ret, ret==0 );
    expected_timeout_ts.tv_sec = expected_sec;
    expected_timeout_ts.tv_nsec = expected_nsec;
    /*check the passed amount time as expected*/
    complete_nanosec_test(&ts, &expected_timeout_ts);
}

void test_nanosleep(int sec, int nsec, int expected_sec, int expected_nsec){
    int ret;
    struct timespec ts, timeout_ts, expected_timeout_ts, remain;
    
    /*save current time*/
    prepare_time_test(&ts);
    /*wait until timeout exceeded*/
    timeout_ts.tv_sec = 0;
    timeout_ts.tv_nsec= 1000;
    TEST_OPERATION_RESULT( nanosleep(&timeout_ts, &remain), &ret, ret==0 );
    expected_timeout_ts.tv_sec = expected_sec;
    expected_timeout_ts.tv_nsec = expected_nsec;
    /*check the passed amount time as expected*/
    complete_nanosec_test(&ts, &expected_timeout_ts);
}

void test_select_timeout(int sec, int nsec, int expected_sec, int expected_nsec){
    int ret;
    struct timespec ts, expected_timeout_ts;
    
    /*save current time*/
    prepare_time_test(&ts);
    /*wait until timeout exceeded*/
    struct timeval timeout_tv;
    timeout_tv.tv_sec = sec;
    timeout_tv.tv_usec = nsec/1000;
    TEST_OPERATION_RESULT( select(0, NULL, NULL,
				  NULL, &timeout_tv), &ret, ret==0);
    expected_timeout_ts.tv_sec = expected_sec;
    expected_timeout_ts.tv_nsec = expected_nsec;
    /*check the passed amount time as expected*/
    complete_nanosec_test(&ts, &expected_timeout_ts);
}

/*various time functions tests for better tests coverage*/
void test_issue_128(){
    int ret;
    struct timezone tz;
    struct timezone *tz_p=NULL; //pass null pointer instead NULL, to avoid warnings
    struct timeval *tv_p=NULL;
    TEST_OPERATION_RESULT( gettimeofday(tv_p, &tz), &ret, ret!=0&&errno==EFAULT);
    TEST_OPERATION_RESULT( gettimeofday(tv_p, tz_p), &ret, ret!=0&&errno==EFAULT);
    test_clock();
}


int main(int argc, char **argv)
{
    int ret;
    test_issue_128();

    test_sleep(1, 0, 1, 0);

    test_nanosleep(0, 0, 0, 1000); /*if 0 time is passed it should take anyway 1 microsecond*/
    test_nanosleep(0, 1998, 0, 1998);

    test_select_timeout(0, 0, 0, 1000);
    test_select_timeout(0, 1000, 0, 1000);

    {
	struct timespec ts, ts2;
	struct timeval tv_inc, tv, tv2;

	/*compare time before and after select, measure in nanoseconds*/
	TEST_OPERATION_RESULT( clock_gettime(CLOCK_REALTIME, &ts), &ret, ret==0 );

	/*select can only wait for timeout when no file descriptors specified*/
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 1000;
	TEST_OPERATION_RESULT( select(0, NULL, NULL,
				      NULL, &timeout), &ret, ret==0);
	timeout.tv_usec += 1; /*clock_gettime done +1 microsecond*/
	TEST_OPERATION_RESULT( clock_gettime(CLOCK_REALTIME, &ts2), &ret, ret==0 );

	TIMESPEC_TO_TIMEVAL(&tv, &ts);
	TIMESPEC_TO_TIMEVAL(&tv2, &ts2);
	timeradd(&tv, &timeout, &tv_inc);

	TEST_OPERATION_RESULT( timercmp(&tv_inc, &tv2, == ), &ret, ret==1);

	/*select's synchronous multiplexing is not supported*/
	fd_set readfds;
	FD_ZERO (&readfds);
	FD_SET (0, &readfds);
	TEST_OPERATION_RESULT( select(FD_SETSIZE, &readfds, NULL,
				      NULL, &timeout), &ret, errno==ENOSYS&&ret!=0);
    }

    return 0;
}

