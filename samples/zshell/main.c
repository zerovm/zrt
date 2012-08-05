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

/* If provided by manifest, this channel contains data and can be accessed in read/write modes */
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

/*@return new line pos offset*/
int SearchNewLine( const char *buffer ){
	const char* bufpos = strchrnul(buffer, '\n' );
	if ( bufpos != NULL && bufpos != buffer ){
		return bufpos - buffer +1;
	}
	else
		return -1;
}

int main(int argc, char **argv){
	/* Read to buffer from stdin which contains text script; first line should always begin on '#';
	 * First line can be one of these: #lua, #sqlite  */
	int fdscript = 0; //stdin
#ifdef DEBUGGER
	fdscript = open( "/home/yaroslav/git/zrt/samples/zshell/sqlite/scripts/zerovm_config.sql", O_RDONLY );
	assert( fdscript >= 0 );
#endif

	char *buffer = NULL;
	int bufsize = LoadFileToBuffer(fdscript, &buffer );                /*load script from stdin*/
	int newlinepos = SearchNewLine( buffer );
	WRITE_FMT_LOG("newlinepos=%d, bufsize=%d\n", newlinepos, bufsize);

	/*Detect type of script lua / sqlite*/
	int errcode = 0;
	if ( !strncmp( LUA_ID, buffer, strlen(LUA_ID) ) && newlinepos >0 ){
#ifndef DEBUGGER
		/*if loaded file is lua script, run script skiping first line; Alternatively it can open
		 * channel contains data which can be accessed in read/write modes */
		int script_buf_size = bufsize-newlinepos;
		errcode = run_lua_buffer_script (buffer+newlinepos, script_buf_size, (const char **)argv);
#endif
	}
	else if ( !strncmp( SQLITE_ID, buffer, strlen(SQLITE_ID) ) && newlinepos > 0 ){
		/*if loaded file is sqlite query, run query skiping first line*/
		int script_buf_size = bufsize-newlinepos;
		int database_fd = open( DATA_CHANNEL_ALIAS_NAME, O_RDONLY );
		WRITE_FMT_LOG( "database_fd=%d\n", database_fd );
		errcode = run_sql_query_buffer( database_fd, buffer+newlinepos, script_buf_size );
		close(database_fd);
	}
	else{
		WRITE_LOG( "Could not detect script type, first line should be started by #SCRIPT_ID" );
	}

	free(buffer);
	return errcode;
}

