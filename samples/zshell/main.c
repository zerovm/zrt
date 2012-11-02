/*
 * zshell_main.c
 *
 *  Created on: 24.07.2012
 *      Author: yaroslav
 */

#include <string.h> //memcpy
#include <stdio.h> //fprintf
#include <stdlib.h> //malloc getenv
#include <unistd.h> //ssize_t
#include <assert.h> //assert
#include <sys/types.h> //temp read file
#include <sys/stat.h> //temp read file
#include <fcntl.h> //temp read file
#include "zshell.h"


#define STDIN 0

/* Channel contains input data allowed to read */
#  define DATA_CHANNEL_ALIAS_NAME "/dev/input"

#define LUA_ID            "#lua"
#define SQLITE_ID         "#sqlite"

/**@param fd file decriptor id to load contents
 * @param buffer pointer to resulted buffer pointer
 * @return loaded bytes count*/
int LoadFileToBuffer( int fd, char **buffer ){
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

/*skip first line contents started with zshell keyword '#' and
 *get position of string with beginning of significant script data
 *@return new line pos offset*/
int SearchNewLine( const char *buffer ){
    const char* bufpos = strchrnul(buffer, '\n' );
    if ( bufpos != NULL && bufpos != buffer ){
	return bufpos - buffer +1;
    }
    else
	return -1;
}

int zmain(int argc, char **argv){
    int fdscript = STDIN;
    char *buffer = NULL;
    /* Read from stdin channel text script; first line in script
     * should always begin with '#'; and can be one of these: 
     * #lua 
     * #sqlite  */

    /*load script from stdin*/
    int bufsize = LoadFileToBuffer(fdscript, &buffer );
    /*skip first line containing keyword*/
    int newlinepos = SearchNewLine( buffer );

    /*Detect script type, comparing keyword and buffer beginning
     *newlinepos is saying where script significant data begins
     and if newlinepos value=0 that is means that no scipt data found */
    int errcode = 0;
    int script_buf_size = bufsize-newlinepos;
    const char* script_contents = buffer+newlinepos; /*whole script skipping first line*/

    /*Filesystem in memory notes:
     *Any folder contents can be injected into filesystem in memory restricted by memory
     *capacity, to do this need to specify /dev/tarimage channel in manifest file and
     *then all archive contents will be copied into '/' FS in memory at nexe start*
     */
    /*Channels notes: 
     *Channels defined in manifest file should be resided in /dev folder; currently 
     *lua can use any types of channels supported by zerovm,
     *- sqlite can use folowing channel types if READ_ONLY_SQL not defined: 
     *RGetSPut=1 Random get / sequential put - for reading database, 
     *SGetRPut=2 Sequential get / Random put - for writing database
     *- sqlite can use only channels with READ ability if READ_ONLY_SQL defined
     *because it using VFS supported only read only channels*/

    /*If LUA scripting provided*/
    if ( !strncmp( LUA_ID, buffer, strlen(LUA_ID) ) && 
	 newlinepos >0 ){
	/*if loaded file is lua script, skip first line and run script;*/
	errcode = 
	    run_lua_buffer_script ( script_contents,   /*buffer with lua script */
				    script_buf_size,   /*length of script*/
				    (const char **)argv /*optional cmd line*/ );
    }

    /*For SQLITE we are waiting an argv[1] param and interpret it as DB filename*/
    else if ( !strncmp( SQLITE_ID, buffer, strlen(SQLITE_ID) ) && 
	      newlinepos > 0 ){
	if ( !argv[1] ){
	    fprintf( stderr, "SQLite required db filename as cmd line argument");
	}
	else{
	    errcode = 
		run_sql_query_buffer( 
				     argv[1],         /*db filename*/
				     script_contents, /*pos starting with script data*/
				     script_buf_size  /*script data size*/
				      );
	}
    }
    else{
	fprintf( stderr, 
		 "Could not detect script type, "
		 "first line should be started by %s or %s", 
		 LUA_ID, SQLITE_ID );
    }
    
    free(buffer);
    return errcode;
}

