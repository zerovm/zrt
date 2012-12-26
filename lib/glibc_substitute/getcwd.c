/*
 * getcwd.c
 * Simplified implementation that substitude glibc stub implementation
 * Just get root path.
 *
 *  Created on: 24.12.2012
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
#include "zrt_helper_macros.h"

#define DEFAULT_WORKING_DIRPATH "/"

char *getcwd(char *buf, size_t size)
{
    if ( !buf ) return NULL;

    ZRT_LOG(L_SHORT, "buf=%s, size=%d", buf, size);
    
    /*buf size is not enough */
    if ( size < sizeof DEFAULT_WORKING_DIRPATH ){
	SET_ERRNO(ERANGE);
    }
    /*buf size is enough to set path "/"  */
    else{
	/*copy into buffer exactly dir of the specified size*/
	strcpy(buf, DEFAULT_WORKING_DIRPATH);
    }

    ZRT_LOG(L_SHORT, "return buf=%s, len=%d", buf, strlen(DEFAULT_WORKING_DIRPATH) );
    return buf;
}
