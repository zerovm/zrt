/*
 * seek demo tests
 */
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include "zrt.h"

int main(int argc, char **argv)
{
    FILE *f = fopen( "seeker.data", "w" );
    assert( f>0 );
    const char *str = "hip hop ololey";
    const char *str2 = "opa-opa-opa-pa";
    const char *str3 = "hip hop opa-opa-opa-pa";
    fwrite(str, 1, strlen(str), f);
    fseek( f, 0, SEEK_CUR );
    assert( ftell(f) == strlen(str) );

    fseek( f, -6, SEEK_CUR );
    assert( ftell(f) == strlen(str)-6 );

    fwrite(str2, 1, strlen(str2), f );
    fseek( f, 0, SEEK_END );
    assert( ftell(f) == strlen(str3) );
    fclose(f);


    int fd = open( "seeker2.data", O_CREAT | O_WRONLY );
    assert( fd >= 0 );
    write(fd, str, strlen(str));
    off_t off;
    off = lseek( fd, 0, SEEK_CUR );
    assert( off == strlen(str) );

    off = lseek( fd, -6, SEEK_CUR );
    assert( off == strlen(str)-6 );

    write(fd, str2, strlen(str2) );
    off = lseek( fd, 0, SEEK_END );
    assert( off == strlen(str3) );
    close(fd);
    return 0;
}


