/*
 * Description
 *
 * Copyright (c) 2013, LiteStack, Inc.
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

#include <stdio.h>
#include <string.h>
#include <assert.h>


extern const char *__progname;

int main(int argc, char **argv)
{
    int i;
    int res;

    /* print list of main() arguments */
    printf("list of arguments%s\n", argc == 1 ? " is empty" : ":");
    fflush(0);

    /*expecting some arguments*/
    if ( argc <= 1 ){
	return 1;
    }

    for(i = 0; i < argc; ++i){
        printf("%d. %s\n", i, argv[i]);
	if ( i == 0 ){
            res = strcmp(argv[i], "command_line.nexe" );
            assert( res == 0 );
	}
        if ( i == 1 ){
            res = strcmp(argv[i], "argument_number_1" );
            assert( res == 0 );
        }
        else if ( i == 2 ){
            res = strcmp(argv[i], "argument_number_2" );
            assert( res == 0 );
        }
        else if ( i == 5 ){
            res = strcmp(argv[i], "the_last_argument" );
            assert( res == 0 );
        }
	else if ( i == 6 ){
            res = strcmp(argv[0], __progname );
	    assert( res == 0 );
	}
    }

    return 0;
}
