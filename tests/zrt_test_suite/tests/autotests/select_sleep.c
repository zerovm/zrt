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


int main(int argc, char **argv)
{
    int ret;
    {
	struct timeval  tv, tv2, tv3;
	struct timezone tz;

	/*compare time before and after sleep, measure in microseconds*/
	TEST_OPERATION_RESULT( gettimeofday(&tv, &tz), &ret, ret==0 );
	int seconds = 1;
	TEST_OPERATION_RESULT( sleep(seconds), &ret, ret==0);
	TEST_OPERATION_RESULT( gettimeofday(&tv2, &tz), &ret, ret==0);
	TEST_OPERATION_RESULT( gettimeofday(&tv3, &tz), &ret, ret==0);
	TEST_OPERATION_RESULT( timercmp(&tv2, &tv3, < ) , &ret, ret==1);
	TEST_OPERATION_RESULT( tv2.tv_sec == tv.tv_sec+seconds, &ret, ret==1);
    }

    {
	struct timespec req, rem;
	struct timespec tp, tp2, tp3;

	/*compare time before and after nanosleep, measure in nanoseconds*/
	TEST_OPERATION_RESULT( clock_gettime(CLOCK_REALTIME, &tp), &ret, ret==0 );
	req.tv_sec = 0;
	req.tv_nsec = 1999;
	TEST_OPERATION_RESULT( nanosleep(&req, &rem), &ret, ret==0 );
	TEST_OPERATION_RESULT( clock_gettime(CLOCK_REALTIME, &tp2), &ret, ret==0 );
	TEST_OPERATION_RESULT( clock_gettime(CLOCK_REALTIME, &tp3), &ret, ret==0 );
	TEST_OPERATION_RESULT( tp2.tv_sec != tp3.tv_sec || tp2.tv_nsec != tp3.tv_nsec, &ret, ret==1);
	TEST_OPERATION_RESULT( tp2.tv_nsec == tp.tv_nsec+req.tv_nsec+1, &ret, ret==1);
    }

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

