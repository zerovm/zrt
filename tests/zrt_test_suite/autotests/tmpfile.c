/*
 * tmpfile using
 * linking issue: 
 * libzrt.a should be added added twice into list of linking options that describes 
 * order of libraries: before and after of object file that using tmpfile function.
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

int main(int argc, char **argv)
{
    int rc=0;
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


