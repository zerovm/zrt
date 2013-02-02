/*patches locale magic number making from newest locale an old version
 *it is a tricky and may not works*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#define FILENAME argv[1]

/*errors handling macroses*/
#define PARAM_EXPECTED "compiled locale filename is expected, like LC_CTYPE"
#define CANT_OPEN_FILE "passed argument is not a file"
#define LOCALE_SKIPPED "expected LC_CTYPE locale file, skipped "
#define RETURN_ERROR(errtext) {					   \
	fprintf(stderr, "error: argv[1]:'%s' %s\n",		   \
		FILENAME, errtext);				   \
	return -1;						   \
    }
#define RETURN_WARNING(warntext) {					\
	fprintf(stderr, "warning: argv[1]:'%s' %s\n",			\
		FILENAME, warntext);					\
	return 0;							\
    }

/*check locale filename parameter*/
#define CHECK_PARAM if ( argc < 2 ) RETURN_ERROR( PARAM_EXPECTED );
#define VALIDATE_FILE_DESCRIPTOR(fd) if ( fd <= 0 ) \
	RETURN_ERROR( CANT_OPEN_FILE );
#define VALIDATE_READED_MAGIC_VALUE(magic_value)			\
    if ( (magic_value ^ REPLACE_THAT) != 0 ){				\
	snprintf( s_buffer, sizeof(s_buffer),				\
		  "%s, fist 4bytes=0x%X", LOCALE_SKIPPED, magic_value  );	\
	RETURN_WARNING( s_buffer );					\
    }

/*magic numbers*/
/*new version of locale*/
#define REPLACE_THAT   0x20090720
/*will become old version of locale*/
#define REPLACE_RESULT 0x20031115

char s_buffer[0x1000];

int main(int argc, char **argv){
    /*argv[1] is expected, waiting file name of compiled locale, 
     *filename starting with LC_*/
    CHECK_PARAM;

    /*open file check current magic number and if it's matched then patch file
     *and save changes*/

    /*open file and "return -1" if file descriptor not valid*/
    int fd = open(argv[1], O_RDWR);
    VALIDATE_FILE_DESCRIPTOR(fd);

    /*read 4byte value and "return 0" if it's not expected*/
    unsigned int magic_value;
    int err = read(fd, &magic_value, sizeof(unsigned int));
    assert(err==4);
    VALIDATE_READED_MAGIC_VALUE(magic_value);

    /*reposition cursor to begin of file and patch first 4 bytes*/
    err = lseek(fd, 0, SEEK_SET);
    assert(err==0);
    magic_value = REPLACE_RESULT;
    err = write(fd, &magic_value, sizeof(unsigned int));
    assert(err=sizeof(magic_value));
    close(fd);
    printf("file '%s' patched.\n", FILENAME);

    return 0;
}
