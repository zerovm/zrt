
//#define __USE_GNU
//#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h"

#define RETURN(ret){							\
	LOG_SHORT_SYSCALL_FINISH(ret,"path=%s, mode=%o(octal)", path, (uint32_t)mode); \
	if ( ret != 0 ){						\
	    SET_ERRNO(errno);						\
	}								\
	return ret;							\
    }

int euidaccess(const char *path, int mode){
    LOG_SYSCALL_START("path=%p, mode=%o(octal)", path, (uint32_t)mode);
    struct stat stats;

    unsigned int granted;

    if ( !stat (path, &stats) ){
	/* The user can read and write any file, and execute any file
	   that anyone can execute.  */
	if ( ((mode & X_OK) == 0)  || (stats.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) )
	    RETURN(0);

	/* Convert the mode to traditional form, clearing any bogus bits.  */
	if (R_OK == 4 && W_OK == 2 && X_OK == 1 && F_OK == 0)
	    mode &= 7;
	else
	    mode = ((mode & R_OK ? 4 : 0)
		    + (mode & W_OK ? 2 : 0)
		    + (mode & X_OK ? 1 : 0));

	if (mode == 0)
	    RETURN(0);			/* The file exists.  */

	/* Convert the file's permission bits to traditional form.  */
	if (S_IRUSR == (4 << 6) && S_IWUSR == (2 << 6) && S_IXUSR == (1 << 6)
	    && S_IRGRP == (4 << 3) && S_IWGRP == (2 << 3) && S_IXGRP == (1 << 3)
	    && S_IROTH == (4 << 0) && S_IWOTH == (2 << 0) && S_IXOTH == (1 << 0))
	    granted = stats.st_mode;
	else
	    granted = ((stats.st_mode & S_IRUSR ? 4 << 6 : 0)
		       + (stats.st_mode & S_IWUSR ? 2 << 6 : 0)
		       + (stats.st_mode & S_IXUSR ? 1 << 6 : 0)
		       + (stats.st_mode & S_IRGRP ? 4 << 3 : 0)
		       + (stats.st_mode & S_IWGRP ? 2 << 3 : 0)
		       + (stats.st_mode & S_IXGRP ? 1 << 3 : 0)
		       + (stats.st_mode & S_IROTH ? 4 << 0 : 0)
		       + (stats.st_mode & S_IWOTH ? 2 << 0 : 0)
		       + (stats.st_mode & S_IXOTH ? 1 << 0 : 0));

	if ((mode & ~granted) == 0)
	    RETURN(0);
	SET_ERRNO(EACCES);
	RETURN(0);
    }
    else{
	/*nacl stat reset errno to EPERM, even if return ENOENT */
	if ( errno == EPERM ){
	    errno = ENOENT;
	}

	SET_ERRNO(errno);
    }
    RETURN(-1);
}

int euidaccessx(const char *path, int mode){
    return euidaccess(path, mode);
}

int eaccess(const char *path, int mode){
    return euidaccess(path, mode);
}

