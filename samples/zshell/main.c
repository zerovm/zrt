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

#include "zrt.h"
#include "zshell.h"

#define SCRIPT_ALIAS_NAME "script.zsh"

int main(int argc, char **argv, char **envp){

	char *test = getenv("TEST");
	WRITE_FMT_LOG( "getenv(TEST)=%s\n", test );

	int i = 0;
	char *env = NULL;
	do{
		env = envp[i++];
		WRITE_FMT_LOG( "envp[%d]=%s\n", i, env );
	}while(env);

//	int fd = open(SCRIPT_ALIAS_NAME, O_RDONLY);
//	char *s = calloc(1, 111111111);
//	int readed = read( fd, s, 11111111 );
//	close(fd);
//	WRITE_FMT_LOG("readed=%d\n", readed);
//	WRITE_LOG( s );
//	free(s);

//	char *s = calloc(1, 111111111);
//	FILE *f =fopen(SCRIPT_ALIAS_NAME, "r");
//	int readed = fread( s, 1, 1, f );
//	WRITE_FMT_LOG("readed=%d\n", readed);
//	WRITE_LOG( s );
//	int readstatus = ferror(f);
//	WRITE_FMT_LOG("ferror=%d\n", readstatus);
//	free(s);
//	fclose(f);
	return run_lua_script( SCRIPT_ALIAS_NAME );
}

