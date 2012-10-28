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


#ifndef DEBUGGER
#include "zrt.h"

/* Channel contains input data allowed to read */
#  define DATA_CHANNEL_ALIAS_NAME "/dev/input"
#else
#  define WRITE_FMT_LOG(fmt, args...)
#  define WRITE_LOG(str) printf("%s\n", str)
#  define DATA_CHANNEL_ALIAS_NAME "/home/yaroslav/git/zrt/samples/zshell/sqlite/data/new.db"
#endif



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
    /* Read to buffer from stdin which contains text script; first line should
     * always begin with '#'; First line should be one of these: 
     * #lua 
     * #sqlite  */
    int fdscript = 0; //stdin
#ifdef DEBUGGER
    fdscript = open( "/home/yaroslav/git/zrt/samples/zshell/sqlite/scripts/zerovm_config.sql", O_RDONLY );
    assert( fdscript >= 0 );
#endif

    char *buffer = NULL;
    /*load script from stdin*/
    int bufsize = LoadFileToBuffer(fdscript, &buffer );
    /*skip first line containing keyword*/
    int newlinepos = SearchNewLine( buffer );

    /*Detect script type, comparing keyword and buffer beginning
     *newlinepos is saying where script significant data begins
     and if newlinepos value=0 that is means that no scipt data found */
    int errcode = 0;
    int script_buf_size = bufsize-newlinepos;
    if ( !strncmp( LUA_ID, buffer, strlen(LUA_ID) ) && 
	 newlinepos >0 ){
#ifndef DEBUGGER
	/*if loaded file is lua script, run script skiping first line;*/
	errcode = 
	    run_lua_buffer_script (buffer+newlinepos, 
				   script_buf_size, 
				   (const char **)argv);
#endif
    }
    else if ( !strncmp( SQLITE_ID, buffer, strlen(SQLITE_ID) ) && 
	      newlinepos > 0 ){
	/*if loaded file is sqlite query, run query skiping first line*/
#ifdef READ_ONLY_SQL
	/*ZSHELL is supported mode working without filesytem available
	 *in this case Database can be opened in READ_ONLY mode
	 *and only native read queries are available*/

	errcode = 
	    run_sql_query_buffer( 
				 DATA_CHANNEL_ALIAS_NAME, /*read-only zerovm channel*/
				 buffer+newlinepos, /*pos starting with script data*/
				 script_buf_size  /*script data size*/
				  );
#else
	errcode = 
	    run_sql_query_buffer( 
				 /*path on tarball FS loaded by manifest*/
				 "/sqlite/data/zvm_netw.db", 
				 buffer+newlinepos, /*pos starting with script data*/
				 script_buf_size  /*script data size*/
				  );
#endif
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

