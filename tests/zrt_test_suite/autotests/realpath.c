/*
 * realpath using
 */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int zmain(int argc, char **argv)
{
    char resolved_path[PATH_MAX];
    char* res = realpath( "realpath.nexe", resolved_path);
    
    if ( res ){
	fprintf(stderr, "realpath =%s\n", res);
    }
    else{
	fprintf(stderr, "realpath failed, errno=%d\n", errno );
    }

    return !res;
}


