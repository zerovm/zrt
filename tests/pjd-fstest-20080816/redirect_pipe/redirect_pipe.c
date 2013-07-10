#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include "unistd.h"
#include <fcntl.h>
#include <error.h>
#include <errno.h>

#ifndef EXIT_FAILURE
#define EXIT_FAILURE -1
#endif

#define USAGE_FORMAT				\
    "Usage: %s -f filename -[r|w]\n"		\
    "-f filename\n"				\
    "-r : Read file and send to STDOUT\n"	\
    "-w : Write STDIN data to file\n"		\
    "-? : This help\n"

#define BAD_FILE "Bad file"

#define IGNORE_READ_ERROR 1
#define HANDLE_READ_ERROR 0

enum IOPipeType{EReadFileToStdout, EWriteStdinToFile, EUninitialized};

enum {EBadFile, EUsage};

static char* program_name;

void raise_error(int err){
    switch(err){
    case EBadFile:
	error(EXIT_FAILURE, ENOENT, BAD_FILE );
	break;
    case EUsage:
    default:
	error(EXIT_FAILURE, EINVAL, USAGE_FORMAT, program_name );
	break;
    }
}

void redirect(FILE* in, FILE* out, int ignore_read_error){
    char chr;
    int c = fread(&chr, 1, 1, in);
    while ( ignore_read_error || (!feof(in)) ){
	if ( c>0 ){
	    fwrite(&chr, 1, 1, out);
	    fflush(out);
	}
	c = fread(&chr, 1, 1, in);
    }
}


/*redirect stdin input into file*/
int main(int argc, char *argv[] ){
    FILE* f = NULL;
    const char* fname = NULL;
    enum IOPipeType t = EUninitialized;
    int opt;
    program_name = argv[0];
    while ((opt = getopt(argc, argv, "f:rw?")) != -1) {
	switch (opt) {
	case 'f':
	    fname = optarg;
	    break;
	case 'r':
	    t = EReadFileToStdout;
	    break;
	case 'w':
	    t = EWriteStdinToFile;
	    break;
	case '?':
	default:
	    raise_error(EUsage);
	}
    }

    /*validate arguments*/
    if ( t == EReadFileToStdout && fname ){
	f = fopen( fname, "r" );
    }
    else if ( t == EWriteStdinToFile && fname ){
	f = fopen( fname, "w" );
    }
    else if (t==EUninitialized || !fname) raise_error(EUsage);
    if ( !f ) raise_error(EBadFile);

    /*validate arguments complete*/
    /*redirect data*/
    if ( t == EReadFileToStdout ){
	/*read file and output into stdout*/
	redirect( f, stdout, IGNORE_READ_ERROR);
    }
    else if ( t == EWriteStdinToFile ){
	/*read stdin and output into file*/
	redirect( stdin, f, HANDLE_READ_ERROR);
    }

    fclose(f);
    return 0;
}
