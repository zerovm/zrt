/*
 * readdir test
 */

#define _GNU_SOURCE

#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <assert.h>
#include "zrt.h"

struct linux_dirent {
    long           d_ino;
    off_t          d_off;
    unsigned short d_reclen;
    char           d_name[];
};
//#define BUF_SIZE 1024
#define BUF_SIZE 100 //small buffer tests


int
syscall_listdir(const char *path)
{
    int fd, nread;
    char buf[BUF_SIZE];
    struct linux_dirent *d;
    int bpos;
    char d_type;

    printf("syscall_listdir(%s)\n", path);
    fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd == -1)
        return -1;

#define NaCl_getdents(handle, buf, count) ((int32_t (*)()) \
        (23 * 0x20 + 0x10000))(handle, buf, count)

    for ( ; ; ) {
        nread = NaCl_getdents(fd, buf, BUF_SIZE);
        if (nread < 0)
            return -1;

        if (nread == 0)
            break;

        printf("--------------- nread=%d ---------------\n", nread);
        printf("i-node#  file type  d_reclen  d_off   d_name\n");
        for (bpos = 0; bpos < nread;) {
            d = (struct linux_dirent *) (buf + bpos);
            printf("%8ld  ", d->d_ino);
            d_type = *(buf + bpos + d->d_reclen - 1);
            printf("%-10s ", (d_type == DT_REG) ?  "regular" :
                    (d_type == DT_DIR) ?  "directory" :
                            (d_type == DT_FIFO) ? "FIFO" :
                                    (d_type == DT_SOCK) ? "socket" :
                                            (d_type == DT_LNK) ?  "symlink" :
                                                    (d_type == DT_BLK) ?  "block dev" :
                                                            (d_type == DT_CHR) ?  "char dev" : "???");
            printf("%4d %10lld %20s \n", d->d_reclen,
                    (long long) d->d_off, (char *) d->d_name);
            bpos += d->d_reclen;
        }
    }

    return 0;
}


int listdir(const char *path) {
    struct dirent *entry;
    DIR *dp;
    printf("listdir(%s)\n", path);
    dp = opendir(path);

    if (dp == NULL) {
        perror("opendir: Path does not exist or could not be read.");
        return -1;
    }

    while ( (entry = readdir(dp))){
        printf( "d_ino=%u, d_off=%u, d_reclen=%u, d_name=%s, ",
                (uint32_t)entry->d_ino, (uint32_t)entry->d_off, entry->d_reclen, entry->d_name );
        printf("%-10s ", (entry->d_type == DT_REG) ?  "regular" :
                (entry->d_type == DT_DIR) ?  "directory" :
                        (entry->d_type == DT_FIFO) ? "FIFO" :
                                (entry->d_type == DT_SOCK) ? "socket" :
                                        (entry->d_type == DT_LNK) ?  "symlink" :
                                                (entry->d_type == DT_BLK) ?  "block dev" :
                                                        (entry->d_type == DT_CHR) ?  "char dev" : "???");
        puts("");
    }

    closedir(dp);
    return 0;
}



int main(int argc, char **argv)
{
    listdir("/dev");fflush(0);
    syscall_listdir("/dev");fflush(0);
    listdir("/");fflush(0);
    syscall_listdir("/");fflush(0);
    listdir(".");fflush(0);
    syscall_listdir(".");fflush(0);

    return 0;
}


