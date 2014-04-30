/*
 * tmpfile test
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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <assert.h>
#include <stdio.h>

#include "macro_tests.h"
#include "parse_path.h" //mkpath_recursively
#include "path_utils.h" //INIT_TEMP_CURSOR, path_component_backward

int main(int argc, char **argv)
{
    int rc=0;
    mkdir("/tmp", 777);
    FILE* tmp = tmpfile();
    if ( tmp ){
	fprintf(stderr, "tmpfile created\n");
	fclose(tmp);
    }
    else{
	fprintf(stderr, "tmpfile does not created\n");
	rc =1;
    }
    return rc;
}


