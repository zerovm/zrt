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
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <assert.h>
#include "zrt.h"

struct stat s_temp_stat;

/*return count backslashes count */
int path_deep_level(const char* path){
    int counter=0;
    do{
	path = strchr(path, '/');
	if ( path ){
	    ++path;
	    ++counter;
	}
    }while(path);
    return counter;
}

void print_prefix( int deep_level ){
    int i;
    for (i=0; i<deep_level; i++ ){
	printf("%2$*1$c", (i+1)*2, '|');
    }
}


int recursive_listdir( const char *path) {
    struct dirent *entry;
    DIR *dp;
    int files_count=0;
    
    int deep_level=path_deep_level(path);
    if ( strlen(path) > 1 ) ++deep_level;
    print_prefix(deep_level);
    printf("--%s Folder\n", path);
    dp = opendir(path);

    if (dp == NULL) {
        perror("opendir: Path does not exist or could not be read.");
        return -1;
    }

    while ( (entry = readdir(dp)) ){
	int compound_path_len = strlen(path) + strlen(entry->d_name) + 2;
	char* compound_path = malloc( compound_path_len );
	int len = strlen(path);
	/*construct full filename for current processing file returned by readdir*/
	if ( len > 0 && path[len-1] == '/' ){
	    /*path already contains trailing slash*/
	    snprintf( compound_path, compound_path_len,  "%s%s", path, entry->d_name ); 
	}
	else{
	    /*add slash because subdirs has no that*/
	    snprintf( compound_path, compound_path_len, "%s/%s", path, entry->d_name ); 
	}

	/*retrieve stat for to get file type and size, assert on error
	 stat should not get fail for now*/
	int err = stat( compound_path, &s_temp_stat );
	assert( err == 0 ); 

	if ( S_ISDIR(s_temp_stat.st_mode) ){
	    /*do notoutput directory because it's name will output 
	      by next recursive call*/
	}
	else{
	    ++files_count;
	    print_prefix(path_deep_level(compound_path));
	    printf( "%s, size=%u, ", 
		    compound_path, (uint32_t)s_temp_stat.st_size );
	    switch( s_temp_stat.st_mode&S_IFMT  ){
	    case S_IFREG:
		printf("regular file");
		break;
	    case S_IFCHR:
		printf("character device");
		break;
	    case S_IFBLK:
		printf("block device");
		break;
	    default:
		printf( "%s, %o", "unknown", s_temp_stat.st_mode&S_IFMT );
		break;
	    }
	    printf( "\n" );	
	}
	
	/*for non current directory do listdir*/
	if ( S_ISDIR(s_temp_stat.st_mode) && strcmp(path, compound_path) != 0 ){
	    files_count+=recursive_listdir(compound_path);
	}
	//printf("ok count=%d %s\n", ++i, compound_path);
	free(compound_path);
    }

    closedir(dp);
    return files_count;
}


int main(int argc, char **argv)
{
    /*recursively print filesystem contents*/
    int files = 0;
    files += recursive_listdir("/");fflush(0);
    printf("All files count is %d\n", files);
    return 0;
}
