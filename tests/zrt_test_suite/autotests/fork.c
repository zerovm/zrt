/*
 * this sample demonstrate zrt library - simple way to use libc
 * from untrusted code.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include <zrtapi.h>

#include "channels/test_channels.h"

#define FILENAME getenv("FPATH")
#define MOUNT_CONTENTS "mount\n"
#define REMOUNT_CONTENTS "remount\n"

char* read_file_contents( const char* name, int* datalen ){
#define BUF_LEN_MAX 10000
    int fd = open( name, O_RDONLY );
    if ( fd != -1 ){
	char* contents = NULL;
	contents = calloc(BUF_LEN_MAX, sizeof(char));
	*datalen = read(fd, contents, BUF_LEN_MAX);
	close(fd);
	return contents;
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int res;
    int datalen;
    int i;
    char* contents = read_file_contents( FILENAME, &datalen );
    CMP_MEM_DATA(MOUNT_CONTENTS, contents, strlen(MOUNT_CONTENTS) );

    for ( i=0; i < 10000; i++ ){
	void* c = malloc(100000);
	if ( c != NULL ){
	    free(c);
	}
    }
    
    res = zfork();
    free(contents);
    contents = read_file_contents( FILENAME, &datalen );
    CMP_MEM_DATA(REMOUNT_CONTENTS, contents, strlen(REMOUNT_CONTENTS) );
    free(contents);
    return 0;
}
