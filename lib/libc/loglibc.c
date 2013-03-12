

#include <string.h>

#include "zvm.h"
#include "zcalls_zrt.h"
#include "zrtlog.h"


static int s_debugfd = -1;

int zrt_zcall_loglibc(const char *str){
    /*It also can be used in prolog, because it is using zvm api and only strcmp
     call that is correct for prolog */
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
        zvm_pwrite(s_debugfd, str, strlen(str), 0);
#define SUFFIX_STR_LEN 1
        zvm_pwrite(s_debugfd, "\n", SUFFIX_STR_LEN, 0);
    }
    return 0;
}
