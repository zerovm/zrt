/*
 * realpath using
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


