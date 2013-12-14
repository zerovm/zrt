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
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

int main(int argc, char **argv)
{
    char buf[PATH_MAX];
    char* res = getcwd(buf, PATH_MAX);

    if ( res ){
	fprintf(stderr, "getcwd =%s\n", res);
    }
    else{
	fprintf(stderr, "getcwd failed, errno=%d\n", errno );
    }

    return !res;
}


