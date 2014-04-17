/*
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <limits.h>
#include <errno.h>
#include <error.h>

#include "utils.h"
#include "macro_tests.h"

int main(int argc, char **argv)
{
    int ret;
    uid_t uid;
    struct passwd* pass;

    /*check getuid return some valid value*/
    TEST_OPERATION_RESULT( uid=getuid(), &ret, ret!=-1);

    /*get passwd struct for valid uid*/
    pass = getpwuid( uid );
    TEST_OPERATION_RESULT( pass!=NULL, &ret, ret!=0);

    TEST_OPERATION_RESULT( strcmp(getenv("HOME"), pass->pw_dir ), &ret, ret==0);
    TEST_OPERATION_RESULT( strcmp(pass->pw_name, getenv("LOGNAME")), &ret, ret==0);

    /*get passwd struct for invalid uid*/
    pass = getpwuid( !uid );
    TEST_OPERATION_RESULT( pass==NULL, &ret, ret!=0);

    TEST_OPERATION_RESULT(test_strtouint_nolocale(), &ret, ret==0);
    return 0;
}


