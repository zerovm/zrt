/*
 * bigfile test feature
 */

//#define __USE_LARGEFILE64
//#define __USE_FILE_OFFSET64

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h> //getenv
#include <fcntl.h>
#include <assert.h>
#include "zrt.h"


int main(int argc, char **argv)
{
    int fd = open( "/dev/bigfile", O_WRONLY );
    assert(fd >= 0);

    __off64_t seekpos = 1024*1024*1024;
    seekpos *= 5; /*5 GB*/
    --seekpos;
    __off64_t pos = lseek( fd, seekpos, SEEK_SET);
    fprintf( stderr, "pos=%lld, seek=%lld\n", pos, seekpos );
    assert( pos == seekpos );
    int bytes = write(fd, "1", 1 );
    assert(bytes==1);

    close(fd);
    return 0;
}
