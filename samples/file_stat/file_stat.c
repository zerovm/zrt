/*
 * demonstration of zrt_fstat() (zrt_stat() contain same routine)
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h> //temp read file
#include <sys/stat.h> //temp read file
#include <unistd.h>
#include <fcntl.h> //temp read file
#include <assert.h>
#include <errno.h>
#include "zrt.h"

#define LOGFD stdout

void print_stat_data( struct stat *s ){
	if (S_ISREG(s->st_mode))
		puts("LOGFD is a redirected disk file");
	else if (S_ISCHR(s->st_mode))
		puts("LOGFD is a character device");

	char *mode = NULL;
	switch (s->st_mode & S_IFMT) {
	case S_IFBLK:  mode = "block device\0";            break;
	case S_IFCHR:  mode = "character device\0";        break;
	case S_IFDIR:  mode = "directory\0";               break;
	case S_IFIFO:  mode = "FIFO/pipe\0";               break;
	case S_IFLNK:  mode = "symlink\0";                 break;
	case S_IFREG:  mode = "regular file\0";            break;
	case S_IFSOCK: mode = "socket\0";                  break;
	default:       mode = "unknown?\0";                break;
	}

	fprintf(LOGFD, "st_dev = %lld\n", s->st_dev);
	fprintf(LOGFD, "st_ino = %lld\n", s->st_ino);
	fprintf(LOGFD, "st_mode = %lo (octal), %s\n", (unsigned long) s->st_mode, mode);
	fprintf(LOGFD, "st_nlink = %d\n", s->st_nlink);
	fprintf(LOGFD, "st_uid = %d\n", s->st_uid);
	fprintf(LOGFD, "st_gid = %d\n", s->st_gid);
	fprintf(LOGFD, "st_rdev = %lld\n", s->st_rdev);
	fprintf(LOGFD, "st_size = %lld File size\n", s->st_size);
	fprintf(LOGFD, "st_blksize = %lld Preferred I/O block size\n", s->st_blksize);
	fprintf(LOGFD, "st_blocks = %lld Blocks allocated\n", s->st_blocks);
	fprintf(LOGFD, "st_atime=%sst_mtime=%sst_ctime=%s \n",
	        ctime(&s->st_atime), ctime(&s->st_mtime), ctime(&s->st_ctime));
	fflush(0);

}

int print_fstat( int fd, struct stat*s ){
	int errcode = fstat(fd, s);
	fprintf(LOGFD, "fstat(fd=%d) call %s\n", fd, errcode < 0 ? "failed" : "successful");
	if(errcode < 0) return -1;
	print_stat_data( s );
	return errcode;
}

int main(int argc, char **argv)
{
    const char *fname = NULL;
	struct stat s;
	int i = 0;

    /* get information about stderr stream */
    fprintf(LOGFD, "TEST 1: stderr starting position = %ld\n", ftell(stderr)); fflush(0);
    if ( (i=print_fstat( fileno(stderr), &s )) < 0 ){
        fprintf(LOGFD, "fstat failed %d\n", i) , fflush(0);
    }
    else{ //stat ok
        assert( (s.st_mode & S_IWUSR) == S_IWUSR ); /*it should allow write only access*/
        assert( (s.st_mode & S_IRUSR) != S_IRUSR );
        assert( (s.st_mode & S_IFCHR) == S_IFCHR ); /*it should be character device*/
    }
    /*************************************/

    /* get information about stdin stream */
    fprintf(LOGFD, "TEST 2: stdin starting position = %ld\n", ftell(stdin)); fflush(0);
    if ( (i=print_fstat( fileno(stdin), &s ) ) < 0 ){
        fprintf(LOGFD, "fstat failed %d\n", i), fflush(0);
    }
    else{ //stat ok
        assert( (s.st_mode & S_IRUSR) == S_IRUSR ); /*it should allow read only access*/
        assert( (s.st_mode & S_IWUSR) != S_IWUSR );
        assert( S_ISCHR(s.st_mode) != 0 ); /*it should be character device*/
    }
    /*************************************/

    fname = "readdata";
    if ( stat( fname, &s) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(LOGFD, "TEST 3: %s file stat\n", fname); fflush(0);
        print_stat_data( &s);
        assert( s.st_nlink == 1 );
        assert( S_ISREG(s.st_mode) != 0 ); /*it shoud be file*/
        assert( (s.st_mode & S_IRUSR) == S_IRUSR ); /*it should allow read only access*/
    }

    fname = "writedata";
    if ( stat( fname, &s) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(LOGFD, "TEST 4: %s file stat\n", fname); fflush(0);
        print_stat_data( &s);
        assert( s.st_nlink == 1 );
        assert( S_ISREG(s.st_mode) != 0 );
        assert( (s.st_mode & S_IWUSR) == S_IWUSR ); /*it should allow write only access*/
    }

    fname = "randomwrite";
    if ( stat( fname, &s) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(LOGFD, "TEST 5: %s file stat\n", fname); fflush(0);
        print_stat_data( &s);
        assert( s.st_nlink == 1 );
        assert( S_ISREG(s.st_mode) != 0 ); /*it shoud be file*/
        assert( (s.st_mode & S_IWUSR) == S_IWUSR ); /*it should allow read only access*/
    }

    fname = "/dev/image";
    if ( stat( fname, &s) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(LOGFD, "TEST 6: %s file stat\n", fname); fflush(0);
        print_stat_data( &s);
        assert( s.st_nlink == 1 );
        assert( S_ISBLK(s.st_mode) != 0 );
        /*it should allow read,write access*/
        assert( (s.st_mode & S_IRUSR) == S_IRUSR && (s.st_mode & S_IWUSR) == S_IWUSR );
    }

    fname = "randomread";
    if ( stat( fname, &s) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(LOGFD, "TEST 7: %s file stat\n", fname); fflush(0);
        print_stat_data( &s);
        assert( s.st_nlink == 1 );
        assert( S_ISREG(s.st_mode) != 0 ); /*it shoud be file*/
        assert( (s.st_mode & S_IRUSR) == S_IRUSR ); /*it should allow read only access*/
        assert( s.st_size != 0 ); /*maped file should have size as read only channel*/
    }
	return 0;
}

