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

/**@param fd file decriptor id to load contents
 * @param buffer pointer to resulted buffer pointer
 * @return loaded bytes count*/
int load_file_to_buffer( int fd, char **buffer ){
	int buf_size = 4096;
	*buffer = calloc( 1, buf_size );
	int readed = 0;
	int requested = 0;
	do{
		requested += buf_size - readed;
		readed += read(fd, *buffer+readed, requested);
		if ( readed == requested )
		{
			buf_size = buf_size*2;
			*buffer = realloc( *buffer, buf_size );
			assert( *buffer );
		}
	}while( readed == requested );
	return readed;
}


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
	fprintf(LOGFD, "st_atime = %s\n", ctime(&s->st_atime));
	fprintf(LOGFD, "st_mtime = %s\n", ctime(&s->st_mtime));
	fprintf(LOGFD, "st_ctime = %s\n\n", ctime(&s->st_ctime));
	fflush(0);

}

int print_fstat( int fd ){
	struct stat s;
	int errcode = fstat(fd, &s);
	fprintf(LOGFD, "fstat(fd=%d) call %s\n", fd, errcode < 0 ? "failed" : "successful");
	if(errcode < 0) return -1;
	print_stat_data( &s );
}

int main(int argc, char **argv)
{
	const char *fname = "readdata";
	int i = 0;
//
//    {
//        struct stat s;
//        if ( stat( fname, &s) == -1) {
//            perror("stat");
//            exit(EXIT_FAILURE);
//        }
//        else{
//            print_stat_data( &s);
//        }
//
//    }
//
//    /* get information about stderr stream */
//    fprintf(LOGFD, "TEST 1: stderr starting position = %ld\n", ftell(stderr)); fflush(0);
//    if ( (i=print_fstat( fileno(stderr) )) < 0 ){
//        fprintf(LOGFD, "fstat failed %d\n", i) , fflush(0);
//    }
//    /*************************************/
//
//
//    /* get information about stdin stream */
//    fprintf(LOGFD, "TEST 2: stdin starting position = %ld\n", ftell(stdin)); fflush(0);
//    if ( (i=print_fstat( fileno(stdin) ) ) < 0 ){
//        fprintf(LOGFD, "fstat failed %d\n", i), fflush(0);
//    }
//    /*************************************/
//


    {
        /* get fstat information about file channel, opened by open function*/
        int fd = open( fname, O_RDONLY );
        fprintf(LOGFD, "TEST 3: opened %s, fd= %d\n", fname, fd);fflush(0);
        //if ( (i=print_fstat( fd ) ) < 0 ){
        //	fprintf(LOGFD, "fstat failed %d\n", i), fflush(0);
        //}
        /*read file and get bytes count*/
        char *buffer = NULL;
        int bytes_count = load_file_to_buffer( fd, &buffer );
        fprintf(LOGFD, "\n%s readed bytes_count %d\n", fname, bytes_count );fflush(0);
        close(fd);
        free(buffer);
        /*************************************/
        /*
[OK] fstat call
zrt_open() is called
zrt_fstat() is called
fstat: 3, channel_size=0
zrt_read() is called
read file fd =3 length=4096 readed=18 18bytes=test file1
zrt_close()

[OK] no fstat call
zrt_open() is called
zrt_read() is called
read file fd =3 length=4096 readed=18 18bytes=test file1
zrt_close() is called

         */
    }

//    {
//        /*
//[BAD]
//zrt_open() is called
//zrt_read() is called
//read file fd =3 length=4096 readed=-262
//zrt_close() is called
//         */
//
//        /* open file channel, by open function, try to read file contents, no call to fstat*/
//        int fd = open( fname, O_RDONLY );
//        fprintf(LOGFD, "FAILURE TEST 4: opened %s, fd= %d\n", fname, fd);fflush(0);
//        FILE *f = fdopen(fd, "r");
//        if ( f >0 ){
//            fprintf(LOGFD, "%s opened file stream = %p\n", fname, f);fflush(0);
//            long pos = ftell(f);
//            fprintf(LOGFD, "%s starting position = %ld\n", fname, pos);fflush(0);
//        }
//        else{
//            fprintf(LOGFD, "%s fdopen failed, errno = %d\n", fname, errno);fflush(0);
//        }
//        /*read file and get bytes count*/
//        char *buffer = NULL;
//        int bytes_count = load_file_to_buffer( fd, &buffer );
//        fprintf(LOGFD, "\n%s readed bytes_count %d\n", fname, bytes_count );fflush(0);
//        if ( f ){
//            fprintf(LOGFD, "%s starting position = %ld\n", fname, ftell(f));fflush(0);
//            fclose(f);
//        }
//        close(fd);
//        free(buffer);
//        /*************************************/
//    }

    {
        /* open file channel, by open function, try to read file contents, no call to fstat*/
        FILE *f = fopen( fname, "r" );
        if ( f >0 ){
            fprintf(LOGFD, "FAILURE TEST 5: %s fopen stream = %p, filefd=%d\n", fname, f, fileno(f));fflush(0);
            long pos = ftell(f);
            fprintf(LOGFD, "%s starting position = %ld\n", fname, pos);fflush(0);

            /*read file and get bytes count*/
            char *buffer = NULL;
            int bytes_count = load_file_to_buffer( fileno(f), &buffer );
            fprintf(LOGFD, "\n%s readed bytes_count %d, errno=%d\n", fname, bytes_count, errno );fflush(0);
            fprintf(LOGFD, "%s starting position = %ld\n", fname, ftell(f));fflush(0);
            fclose(f);
            free(buffer);
        }
        else{
            fprintf(LOGFD, "FAILURE TEST 5: %s fdopen failed, returned pointer = %p\n", fname, f);fflush(0);
        }
    }

//    {
//        /* open file channel, by open function, try to read file contents, no call to fstat*/
//        FILE *f = fopen( fname, "r" );
//        if ( f >0 ){
//            fprintf(LOGFD, "FAILURE TEST 6: %s fopen stream = %p\n", fname, f);fflush(0);
//            long pos = ftell(f);
//            fprintf(LOGFD, "%s starting position = %ld\n", fname, pos);fflush(0);
//
//            /*read file and get bytes count*/
//            size_t buf_size = 100000;
//            char *buffer = calloc( 1, buf_size );
//            int bytes_count = fread( buffer, 1, buf_size, f);
//            fprintf(LOGFD, "\n%s readed bytes_count %d\n", fname, bytes_count );fflush(0);
//            fprintf(LOGFD, "%s starting position = %ld\n", fname, ftell(f));fflush(0);
//            fclose(f);
//            free(buffer);
//        }
//        else{
//            fprintf(LOGFD, "FAILURE TEST 6: %s fdopen failed, returned pointer = %p\n", fname, f);fflush(0);
//        }
//        /*************************************/
//    }
	return 0;
}
