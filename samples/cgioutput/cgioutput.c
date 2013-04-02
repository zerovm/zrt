/*
 * this sample demonstrate zrt library - simple way to use libc
 * from untrusted code.
 */
#include <stdio.h>
#include <stdlib.h>

#include <wchar.h>
#include <assert.h>

int main(int argc, char **argv)
{
    size_t bufsize=0x1000;
    void* buffer = malloc(bufsize);
    int dataread=0;
    do{
	if ( (dataread+=fread(buffer+dataread, 1, bufsize-dataread, stdin )) == bufsize ){
	    bufsize*=2;
	    buffer = realloc(buffer, bufsize);
	}
    }while( !feof(stdin) );
    
    char* content_type = getenv("CONTENT_TYPE");
    if ( !content_type ) content_type = "text/html";

    printf("HTTP/1.1 200 OK \r\n" );
    printf("Content-type: %s\r\n\r\n", content_type );
    fwrite(buffer, 1, dataread, stdout);
    fflush(0);
    return 0;
}
