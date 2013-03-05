

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

#include "zcalls_zrt.h"
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "transparent_mount.h"
#include "mounts_interface.h"
#include "path_utils.h"


int zrt_zcall_loglibc(const char *str){
    if (str && __zrt_log_is_enabled()){
	int debug_handle_123;						
	char *buf__123;							
	if( (debug_handle_123=__zrt_log_debug_get_buf(&buf__123)) >= 0 ){ 
	    int len_123 = snprintf(buf__123, LOG_BUFFER_SIZE,		
				   "ENOSYS on %s\n", str );	
	    __zrt_log_write(debug_handle_123, buf__123, len_123, 0);	
	}								
    }
}
