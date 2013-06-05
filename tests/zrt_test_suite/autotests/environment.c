/*
 * demo of the new user custom attributes design
 */
#include <stdio.h>
#include <stdlib.h> //getenv
#include <string.h>
#include <assert.h>
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
    const char* val = NULL;
    printf("get environment variables, by getenv:\n");
    var = "TimeStamp"; val = getenv(var);  
    if ( val == NULL ) return 1;
    printf("%s=%s\n", var, val );
    assert( !strcmp(val, "1337012520") );
    var = "SafeWords"; val = getenv(var);  
    if ( val == NULL ) return 1;
    printf("%s=%s\n", var, val );
    assert( !strcmp(val, "klaato_verada_nikto") );

    printf("\nTEST3: using setenv & getenv:\n");
    printf( "before new environment assignemnt\n" );
    var = "Pum"; val = getenv(var);  
    if ( val != NULL ) return 1;
    printf("%s=%s\n", var, val );
    printf( "setenv status=%d;\nafter new environment assignment\n",  setenv( "Pum", "PumPum", 1) );
    val = getenv(var); 
    if ( val == NULL ) return 1;
    printf("%s=%s\n", var, val );
    return 0;
}
