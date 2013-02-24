/*
 * normalizing wrong path testing
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#define SLASHES_PATH "//wrng"
#define OK_PATH "/wrng"

int main(int argc, char **argv)
{
    struct stat st;
    int res;
    int fd = open(SLASHES_PATH, O_CREAT|O_RDWR);
    close(fd);
    res = stat(OK_PATH, &st);
    return res;
}
