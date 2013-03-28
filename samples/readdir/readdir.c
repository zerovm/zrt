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

    listdir("/dev");fflush(0);
    listdir("/");fflush(0);
    listdir(".");fflush(0);

    return 0;
}

