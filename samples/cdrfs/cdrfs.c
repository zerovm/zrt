/*
 * this sample demonstrate zrt library - simple way to use libc
 * from untrusted code.
 *
 * in order to use zrt "api/zrt.h" should be included
 */
#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <assert.h>
#include "zrt.h"

struct stat s_temp_stat;
char s_file_name[4096];

int recursive_listdir( const char *path) {
    struct dirent *entry;
    DIR *dp;
    int files_count=0;
    printf("Folder '%s')\n", path);
    dp = opendir(path);

    if (dp == NULL) {
        perror("opendir: Path does not exist or could not be read.");
        return -1;
    }

    while ( (entry = readdir(dp)) ){
	int len = strlen(path);
	/*construct full filename*/
	if ( len > 0 && path[len-1] == '/' ){
	    /*path already contains trailing slahs*/
	    sprintf( s_file_name, "%s%s", path, entry->d_name ); 
	}
	else{
	    /*add slahs because subdirs has no that*/
	    sprintf( s_file_name, "%s/%s", path, entry->d_name ); 
	}

	int err = stat( s_file_name, &s_temp_stat ); /*retrieve stat for file type and size*/
	assert( err == 0 ); /*stat should not fail*/

        printf( "%s [d_ino=%u, d_name=%s, ", s_file_name, 
		(uint32_t)entry->d_ino, entry->d_name );	

	if ( S_ISDIR(s_temp_stat.st_mode) ){
	    printf( "directory" );	
	}
	else{
	    ++files_count;
	    if ( S_ISREG(s_temp_stat.st_mode) ){
		printf( "regular file" );	
	    }
	    else if ( S_ISBLK(s_temp_stat.st_mode) ){
		printf( "character device" );	
	    }
	    else{
		printf( "unknown %d", s_temp_stat.st_mode );	
	    }
	}
	printf( "\n" );	
	
	/*for non current directory do listdir*/
	if ( S_ISDIR(s_temp_stat.st_mode) && strcmp(path, s_file_name) != 0 ){
	    files_count+=recursive_listdir(s_file_name);
	}
    }

    closedir(dp);
    return files_count;
}


int zmain(int argc, char **argv)
{
    /*recursively print filesystem contents*/
    int files = 0;
    files += recursive_listdir("/");fflush(0);
    files += recursive_listdir("/dev");fflush(0);
    printf("All files count is %d\n", files);
    return 0;
}
