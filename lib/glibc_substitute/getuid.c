/*
 * getuid.c
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


#include "zrtlog.h"


uid_t getuid(void)
{
    uid_t userid = 1;
    ZRT_LOG(L_SHORT, "userid=%u", userid);
    return userid;
}
