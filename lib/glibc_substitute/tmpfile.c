/*
 * fstab_loader.c
 * tmpfile implementation that substitude glibc stub implementation
 *
 *  Created on: 01.12.2012
 *      Author: yaroslav
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"

#define TMPDIR = "/tmp"
#define MAX_PATH 256

/* This returns a new stream opened on a temporary file (generated
   by tmpnam).  The file is opened with mode "w+b" (binary read/write).
   If we couldn't generate a unique filename or the file couldn't
   be opened, NULL is returned.  */
FILE *
tmpfile (void)
{
    ZRT_LOG(L_SHORT, P_TEXT, "glibc substitude call");
    char* temp;
    char tempfilename[MAX_PATH];
    int err;
    FILE *f = NULL;

    memset(tempfilename, '\0', MAX_PATH );

    /* get TMPDIR from environment
     * In case the environment variable TMPDIR exists and contains the name
     * of an appropriate directory, that is used.  */
    if ( (temp=getenv("TMPDIR")) ){
	strcpy(tempfilename, temp);
    }

    /*Otherwise, P_tmpdir (as defined in <stdio.h>) is used when appropriate.*/
    if ( temp == NULL ){	
	temp = P_tmpdir;
	strcpy(tempfilename, P_tmpdir);
    }

    mkdir(tempfilename, 0777);
    assert(temp);
    strcpy(tempfilename+strlen(tempfilename), "/tmp_XXXXXX");

    /* provide template & modify string & create temp file*/
    err = mkstemp(tempfilename);

    /*if no error occured at mkstemp*/
    if (err != -1){
	close(err);

	/* Create an unnamed file in the temporary directory.  */
	f = fopen(tempfilename, "w+b");
    }
    return f;
}
