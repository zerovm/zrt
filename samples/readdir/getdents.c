/*
 * readdir test
 */

#define _GNU_SOURCE

#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <assert.h>

#define LINUX_DIRENT dirent


//#define BUF_SIZE 1024
#define BUF_SIZE 100 //small buffer tests


int
syscall_listdir(const char *path)
{
    int fd, nread;
    char buf[BUF_SIZE];
    struct LINUX_DIRENT *d;
    int bpos;
    char d_type;

    printf("syscall_listdir(%s)\n", path);
    fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd == -1)
        return -1;

    for ( ; ; ) {
        nread = __getdents(fd, buf, BUF_SIZE);
        if (nread < 0)
            return -1;

        if (nread == 0)
            break;

        printf("--------------- nread=%d ---------------\n", nread);
        printf("i-node#  file type  d_reclen  d_off   d_name\n");
        for (bpos = 0; bpos < nread;) {
            d = (struct LINUX_DIRENT *) (buf + bpos);
            printf("%8ld  ", d->d_ino);
            d_type = d->d_type;
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


int main(int argc, char **argv)
{
    printf("struct dirent{\n\t"
	   "offset d_ino   =%d\n\t"
	   "offset d_off   =%d\n\t"
	   "offset d_reclen=%d\n\t"
	   "offset d_name  =%d\n"
	   "}, sizeof = %d\n", 
	   offsetof(struct dirent, d_ino ),
	   offsetof(struct dirent, d_off ),
	   offsetof(struct dirent, d_reclen ),
	   offsetof(struct dirent, d_name ),
	   sizeof(struct dirent));

    syscall_listdir("/dev");fflush(0);
    syscall_listdir("/");fflush(0);
    syscall_listdir(".");fflush(0);

    return 0;
}

