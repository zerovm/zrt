

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "zvm.h"
#include "zcalls_zrt.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "path_utils.h"


static int s_debugfd = -1;

int zrt_zcall_loglibc(const char *str){
    if ( s_debugfd < 0 ){
	int i;
	for (i=0; i < MANIFEST->channels_count; i++ ){
	    if ( !strcmp(ZRT_LOG_NAME, MANIFEST->channels[i].name) ){
		s_debugfd = i;
		break;
	    }
	}
    }

    if ( s_debugfd >0 && str ){
	__zrt_log_write(s_debugfd, str, strlen(str), 0);
#define ENOSYS_STR_LEN 11
	__zrt_log_write(s_debugfd, "ENOSYS on \n", ENOSYS_STR_LEN, 0);
    }
}
