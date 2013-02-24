/*trancate functions testing*/

#include <errno.h>
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

//#include "tst-truncate.h"

//functions
#define FTRUNCATE ftruncate
#define TRUNCATE  truncate
#define STRINGIFY(s) #s

//values
#define FILENAME "/truncatefile"
#define EXIT_FAILURE -1

int
main (int argc, char **argv)
{
    int fd = open(FILENAME, O_RDWR|O_CREAT);
    if (fd < 0)
	error (EXIT_FAILURE, errno, "during open");

    char *name = FILENAME;
    struct stat st;
    char buf[1000];

    memset (buf, '\0', sizeof (buf));

    if (write (fd, buf, sizeof (buf)) != sizeof (buf))
	error (EXIT_FAILURE, errno, "during write");

    if (fstat (fd, &st) < 0 || st.st_size != sizeof (buf))
	error (EXIT_FAILURE, 0, "initial size wrong");

    if (FTRUNCATE (fd, 800) < 0)
	error (EXIT_FAILURE, errno, "size reduction with %s failed",
	       STRINGIFY (FTRUNCATE));

    if (fstat (fd, &st) < 0 || st.st_size != 800)
	error (EXIT_FAILURE, 0, "size after reduction with %s incorrect",
	       STRINGIFY (FTRUNCATE));

    /* The following test covers more than POSIX.  POSIX does not require
       that ftruncate() can increase the file size.  But we are testing
       Unix systems.  */
    if (FTRUNCATE (fd, 1200) < 0)
	error (EXIT_FAILURE, errno, "size increase with %s failed",
	       STRINGIFY (FTRUNCATE));

    if (fstat (fd, &st) < 0 || st.st_size != 1200)
	error (EXIT_FAILURE, 0, "size after increase with %s incorrect",
	       STRINGIFY (FTRUNCATE));


    if (TRUNCATE (name, 800) < 0)
	error (EXIT_FAILURE, errno, "size reduction with %s failed",
	       STRINGIFY (TRUNCATE));

    if (fstat (fd, &st) < 0 || st.st_size != 800)
	error (EXIT_FAILURE, 0, "size after reduction with %s incorrect",
	       STRINGIFY (TRUNCATE));

    /* The following test covers more than POSIX.  POSIX does not require
       that truncate() can increase the file size.  But we are testing
       Unix systems.  */
    if (TRUNCATE (name, 1200) < 0)
	error (EXIT_FAILURE, errno, "size increase with %s failed",
	       STRINGIFY (TRUNCATE));

    if (fstat (fd, &st) < 0 || st.st_size != 1200)
	error (EXIT_FAILURE, 0, "size after increase with %s incorrect",
	       STRINGIFY (TRUNCATE));


    close (fd);
    unlink (name);

    return 0;
}
