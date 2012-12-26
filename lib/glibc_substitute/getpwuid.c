/*
 * getpwuid.c
 * Simplified implementation that substitude glibc stub implementation
 * 
 *
 *  Created on: 24.12.2012
 *      Author: yaroslav
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h"


struct passwd s_passwd_data_1 = {
    "username",      /* username */
    "password",      /* user password */
    1,               /* user ID */
    1,               /* group ID */
    "userinfo",      /* user information */
    "/",             /* home directory */
    "zshell.nexe"    /* shell program */
};


struct passwd* getpwuid(uid_t uid)
{
    ZRT_LOG(L_SHORT, "uid=%u", uid);
    if ( uid == 1 )
	return &s_passwd_data_1;
    else{
	SET_ERRNO(ENOENT);    /*uid mismatch*/
    }
    return NULL;
}
