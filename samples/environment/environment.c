/*
 * demo of the new user custom attributes design
 */
#include <stdio.h>
#include <stdlib.h> //getenv
#include "zrt.h"

int main(int argc, char **argv)
{
    extern char **environ;
    char **env = environ;
    printf("environment variables list tests\n");

    printf("\nTEST1: using extern environ:\n");
    while(*env != NULL)
       printf("\"%s\"\n", *env++);

    printf("\nTEST2: using getenv:\n");
    const char* var = NULL;
    printf("get environment variables, by getenv:\n");
    var = "TimeStamp";  printf("%s=%s\n", var, getenv(var) );
    var = "SafeWords";  printf("%s=%s\n", var, getenv(var) );

    printf("\nTEST3: using setenv & getenv:\n");
    printf( "before new environment assignemnt\n" );
    var = "Pum";
    printf("%s=%s\n", var, getenv(var) );
    printf( "setenv status=%d;\nafter new environment assignment\n",  setenv( "Pum", "PumPum", 1) );
    printf("%s=%s\n", var, getenv(var) );
    return 0;
}
