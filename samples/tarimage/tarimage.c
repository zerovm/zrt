/*
 * this sample demonstrate zrt library - simple way to use libc
 * from untrusted code.
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

	if ( entry->d_type != DT_DIR ){
	    /*do notoutput directory because it's name will output 
	      by next recursive call*/
	    ++files_count;
	    print_prefix(path_deep_level(compound_path));
	    /*retrieve stat for to get file size, assert on error
	      stat should not get fail for now*/
	    int err = stat( compound_path, &s_temp_stat );
	    assert( err == 0 ); 

	    printf( "%s, size=%u, ", 
		    compound_path, (uint32_t)s_temp_stat.st_size );
	    switch( entry->d_type  ){
	    case DT_REG:
		printf("regular file");
		break;
	    case DT_CHR:
		printf("character device");
		break;
	    case DT_FIFO:
		printf("pipe");
		break;
	    case DT_BLK:
		printf("block device");
		break;
	    case DT_LNK:
		printf("link");
		break;
	    case DT_SOCK:
		printf("socket");
		break;
	    default:
		printf( "%s, (octal)%o", "unknown", s_temp_stat.st_mode&S_IFMT );
		break;
	    }
	    printf( "\n" );	
	}
	else if ( strcmp(path, compound_path) != 0 ){
	    /*for non current directory do listdir*/
	    files_count+=recursive_listdir(compound_path);
	}
	free(compound_path);
    }

    closedir(dp);
    return files_count;
}


int main(int argc, char **argv)
{
    /*recursively print filesystem contents*/
    int files = 0;
    files += recursive_listdir("/");
    printf("All files count is %d\n", files);
    fflush(0);
    return 0;
}
