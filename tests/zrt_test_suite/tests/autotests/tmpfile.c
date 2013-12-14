/*
 * tmpfile test
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>

/* extern int __path_search (char *__tmpl, size_t __tmpl_len, */
/* 			  __const char *__dir, __const char *__pfx, */
/* 			  int __try_tempdir); */

/* extern int __gen_tempname (char *__tmpl, int __flags, int __kind); */
/* /\* The __kind argument to __gen_tempname may be one of: *\/ */
/* #  define __GT_FILE	0	/\* create a file *\/ */
/* #  define __GT_DIR	1	/\* create a directory *\/ */
/* #  define __GT_NOCREATE	2	/\* just find a name not currently in use *\/ */


/* FILE * */
/* tmpfile (void) */
/* { */
/*     char buf[FILENAME_MAX]; */
/*     int fd; */
/*     FILE *f; */

/*     if (__path_search (buf, FILENAME_MAX, NULL, "tmpf", 0) ){ */
/* 	fprintf(stderr, "test %d\n", 1); */
/* 	return NULL; */
/*     } */
/*     int flags = 0; */
/* #ifdef FLAGS */
/*     flags = FLAGS; */
/* #endif */
/*     fd = __gen_tempname (buf, flags, __GT_FILE); */
/*     if (fd < 0){	 */
/* 	fprintf(stderr, "test %d\n", 2); */
/* 	return NULL; */
/*     } */

/*     /\* Note that this relies on the Unix semantics that */
/*        a file is not really removed until it is closed.  *\/ */
/*     (void) unlink (buf); */
/*     fprintf(stderr, "test %d\n", 3); */

/*     if ((f = fdopen (fd, "w+b")) == NULL){ */
/* 	fprintf(stderr, "test %d\n", 4); */
/* 	close (fd); */
/*     } */

/*     return f; */
/* } */


int main(int argc, char **argv)
{
    int rc=0;
    mkdir("/tmp", 777);
    FILE* tmp = tmpfile();
    if ( tmp ){
	fprintf(stderr, "tmpfile created\n");
	fclose(tmp);
    }
    else{
	fprintf(stderr, "tmpfile does not created\n");
	rc =1;
    }
    return rc;
}


