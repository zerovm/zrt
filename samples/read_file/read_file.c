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
    FILE *f = fopen( "file", "w" );
    assert( f>0 );
    fwrite( "hello\nhell", 1, 10, f );
    fclose( f );
    f = fopen( "file", "r" );
    size_t buf_size = 4096;
    char *buffer = malloc( buf_size );
    size_t r = 0;
    while( (r=fread( buffer, 1, buf_size, f)) != 0 ){
        printf( "readed buf_size=%d\n", r );
    }
    fclose(f);
    return 0;
}


