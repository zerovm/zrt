/*
 * main_test1.c
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 *  REQREP example is simulates two network nodes transmitting data from each to other.
 *  Data packet size=1000000bytes, and it's sending 100times in loop from one node to another and vice versa.
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>

#include "zrt.h"

#define IN_DIR "/dev/in"
#define OUT_DIR "/dev/out"
#define MAX_PATH_LENGTH 255

const char *listdir(DIR* opened_dir) {
    struct dirent *entry;
    assert(opened_dir);

    while ( (entry = readdir(opened_dir)) ){
        return entry->d_name;
    }
    return NULL;
}

int main(int argc, char **argv){
    char path[MAX_PATH_LENGTH];
    const char *name;
    int fdr = -1;
    int fdw = -1;
    char *first = getenv("first");

	WRITE_FMT_LOG("reqrep started, first=%s\n", first);
    if ( strcmp(first, "read") && strcmp(first, "write") ){
        WRITE_LOG("environment variable FIRST as waiting values (read | write)\n");
        exit(0);
    }

	/***********************************************************************
	 open in channel, readed from /dev/in dir */
    DIR *dp = opendir( IN_DIR );
    if ( (name=listdir(dp)) ){
        snprintf( path, MAX_PATH_LENGTH, "%s/%s", IN_DIR, name );
    }
    else{
        assert("can't read in dir");
    }
    closedir(dp);
    fdr = open(path, O_RDONLY);
    WRITE_FMT_LOG("test1: O_RDONLY fda=%d, path=%s\n", fdr, path);

    /***********************************************************************
     open out channels, readed from /dev/out dir */
    dp = opendir( OUT_DIR );
    if ( (name=listdir(dp)) ){
        snprintf( path, MAX_PATH_LENGTH, "%s/%s", OUT_DIR, name );
    }{
        assert("can't read out dir");
    }
    closedir(dp);
    fdw = open(path, O_WRONLY);
    WRITE_FMT_LOG("test1: O_WRONLY fdb=%d, path=%s\n", fdw, path);

    assert( fdr != -1 );
    assert( fdw != -1 );

	int testlen = 1000000;
	char *buf = malloc(testlen+1);
	buf[testlen] = '\0';
	for (int i=0; i < 10; i++){
	    if ( !strcmp(first, "read") ){
            ssize_t bread = read(fdr, buf, testlen);
            WRITE_FMT_LOG("#%d case1: read requested=%d, read=%d\n", i, testlen, (int)bread );
            assert( bread == testlen );
            ssize_t bwrite = write(fdw, buf, testlen);
            WRITE_FMT_LOG("#%d case2: write passed=%d, wrote=%d\n", i, testlen, (int)bwrite );
            assert( bwrite == testlen );
	    }
	    else{
	        ssize_t bwrote = write(fdw, buf, testlen);
	        WRITE_FMT_LOG("#%d case1: write passed=%d, wrote=%d\n", i, testlen, (int)bwrote );
	        assert( bwrote == testlen );
	        ssize_t bread = read(fdr, buf, testlen);
	        WRITE_FMT_LOG("#%d case2: read requested=%d, read=%d\n", i, testlen, (int)bread );
	        assert( bread == testlen );
	    }
	}
	free(buf);

	close(fdr);
	close(fdw);
	WRITE_LOG("exit\n");
	return 0;
}
