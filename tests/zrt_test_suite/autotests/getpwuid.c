/*
 * getpwuid using
 */

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <limits.h>
#include <errno.h>

int zmain(int argc, char **argv)
{
    int rc=0;
    struct passwd* pass = getpwuid(1);   /*valid uid=1*/
    pass == NULL? rc=1 : rc;

    pass = getpwuid(2);                  /*invalid uid=2*/
    pass != NULL? rc=1 : rc;

    fprintf(stderr, "rc=%d, errno=%d\n", rc, errno );
    return rc;
}


