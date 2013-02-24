#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "zrt.h"

int main(int argc, char **argv)
{
    int i;
    int res;

    /* print list of main() arguments */
    printf("list of arguments%s\n", argc == 1 ? " is empty" : ":");
    for(i = 0; i < argc; ++i){
        printf("%d. %s\n", i, argv[i]);
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
    }

    return 0;
}
