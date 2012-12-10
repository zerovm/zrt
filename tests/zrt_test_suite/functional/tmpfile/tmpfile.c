/*
 * tmpfile using
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

int zmain(int argc, char **argv)
{
    FILE* tmp = tmpfile();
    if ( tmp ){
	fprintf(stderr, "tmpfile created\n");
	fclose(tmp);
    }
    else{
	fprintf(stderr, "tmpfile does not created\n");
    }
    assert(tmp);
    return 0;
}


