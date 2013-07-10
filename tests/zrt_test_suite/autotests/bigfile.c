/*
 * bigfile test feature
 */

#define HAVE_ZRT
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h> //getenv
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include "channels/test_channels.h"

int main(int argc, char **argv)
{
    int ret;
    int fd = open64( "/dev/writeonly", O_WRONLY );
    assert(fd >= 0);

    off64_t seekpos = 1024*1024*1024;
    seekpos *= 5; /*5 GB*/
    --seekpos;
    off64_t pos = lseek64( fd, seekpos, SEEK_SET);
    TEST_OPERATION_RESULT(
			  pos == seekpos,
			  &ret, ret!=0 );
    int bytes;
    TEST_OPERATION_RESULT(
			  write(fd, "1", 1 ),
			  &bytes, bytes==1);
    close(fd);
    return 0;
}
