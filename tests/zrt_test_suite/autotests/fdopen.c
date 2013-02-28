/*
 * fdopen using
 */


#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char **argv)
{
    int fd = open("/newfile123", O_CREAT|O_RDWR);

    FILE* f = fdopen(fd, "r");
    fprintf(stderr, "'r', fd=%d, f=%p, errno=%d\n", fd, f, errno );

    FILE* f2 = fdopen(fd, "w");
    fprintf(stderr, "'w', fd=%d, f=%p, errno=%d\n", fd, f2, errno );

    return (int)(!f||!f2);
}


