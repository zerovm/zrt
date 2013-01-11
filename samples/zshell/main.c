/*
 * zshell_main.c
 * ZShell application intended to run script files, currently available support for 
 * LUA, SQLITE, PYTHON scripting;
 * First line of script should point to script type, and can be one of this macroses:
 * FIRSTLINE_LUA_ID / SQLITE_ID / PYTHON_ID
 *
 *  Created on: 24.07.2012
 *      Author: yaroslav
 */

#define _GNU_SOURCE

#include <string.h> //memcpy
#include <strings.h> //strcasecmp
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

#define FIRSTLINE_LUA_ID            "#lua"
#define FIRSTLINE_SQLITE_ID         "#sqlite"
#define FIRSTLINE_PYTHON_ID         "#python"

typedef enum { 
    EWrongScriptType = 0,
    ELuaScriptType,
    ESqliteScriptType,
    EPythonScriptType
} ZshellScriptType;

#define FIRST_LINE_MAX_LEN 20

/*Get script type depending from first line contents, lines count remain unchanged;*/
ZshellScriptType DetectScriptType(FILE* f){
    char first_line[FIRST_LINE_MAX_LEN];  /*first_line contains script type, it's short*/
    char ch;
    int len = 0;

    /* Push back first newline so line numbers remain the same */
    while ( len < FIRST_LINE_MAX_LEN && (ch = getc(f)) != EOF) {
	if (ch == '\n') {
	    (void)ungetc(ch, f);
	    break;
	}
	first_line[len++] = ch;
    }

    /* Detect script type */
    if ( !strncmp( FIRSTLINE_LUA_ID, first_line, strlen(FIRSTLINE_LUA_ID) ) 
	 && len >0 ){
	return ELuaScriptType;
    }
    /*If PYTHON script passed as input*/
    else if ( !strncmp( FIRSTLINE_PYTHON_ID, first_line, strlen(FIRSTLINE_PYTHON_ID) ) 
	      && len >0 ){
	return EPythonScriptType;
    }
    /*For SQLITE we are waiting an argv[1] param and interpret it as DB filename*/
    else if ( !strncmp( FIRSTLINE_SQLITE_ID, first_line, strlen(FIRSTLINE_SQLITE_ID) ) 
	      && len > 0 ){
	return ESqliteScriptType;
    }
    else{
	return EWrongScriptType;
    }
    return EWrongScriptType;
}

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
    int errcode = 0;

    /* Read from stdin channel text script; first line in script
     * should always begin with '#'; and can be one of these: 
     * #lua 
     * #sqlite 
     * #python*/
    ZshellScriptType scr_type = DetectScriptType( stdin );

    /*Filesystem in memory notes:
     *(tempfs is alternative termin, currently any changes will be lost at the termination)
     *Any folder contents can be injected into ZRT filesystem and restricted only by memory
     *capacity, to do this need to specify tarball image and fstab configuration, see docs.
     *Any access types for scripts are allowed if it has at least capability for reading, 
     *All content that not resides in /dev folder is related to filesystem in memory;
     */
    /*Channels notes: 
     *Channels defined in manifest file should be resided in /dev folder.
     *LUA and PYTHON can use any types of channels supported by zerovm.
     *SQLITE can't use random read/random write channels, 
     *TODO: SQLITE on CDR feature; 
     */

    switch( scr_type ){
    case ELuaScriptType:{
	/*load script from stdin*/
	int bufsize = LoadFileToBuffer(fdscript, &buffer );
	/*run script*/
	errcode = run_lua_buffer_script ( buffer, bufsize, (const char **)argv );
	break;
    }

    case ESqliteScriptType:{
	/*load script from stdin*/
	int bufsize = LoadFileToBuffer(fdscript, &buffer );
	/*run script*/
	errcode = run_lua_buffer_script ( buffer, bufsize, (const char **)argv );

	if ( argc < 3 ){
	    fprintf( stderr, "Error:SQLite required cmd line "
		     "1st argument as db filename "
		     "2nd arg string: "
		     "'ro' -readonly DB or "
		     "'rw'- writable DB, without quotes.\n");
	}
	else{
	    int open_mode = EDbWrong;
	    if      ( !strcasecmp("ro", argv[2]) )  open_mode = EDbReadOnly;
	    else if ( !strcasecmp("rw", argv[2]) )  open_mode = EDbReWritable;
	    else{
		fprintf( stderr, 
			 "error: 2nd argument has wrong name '%s'.\n", argv[2] );
	    }
	    run_sql_query_buffer( 
				 argv[1],         /*db filename*/
				 open_mode,       /*db open mode readonly/writable*/
				 buffer,  /*pos starting with script data*/
				 bufsize  /*script data size*/
				  );
	}
	break;
    }
    case EPythonScriptType:
	/*if loaded file is an python script, skip first line and run it;*/
	errcode = run_python_interpreter(argc, argv);
	break;
    default:
    case EWrongScriptType:
	fprintf( stderr, 
		 "Could not detect script type, "
		 "first line should be started by %s or %s", 
		 FIRSTLINE_LUA_ID, FIRSTLINE_SQLITE_ID );
	break;
    }
    
    free(buffer);
    return errcode;
}

