/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#define HAVE_ALLOCA_H 1

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have a working `mmap' system call.  */
#define HAVE_MMAP 1

/* Define if your struct stat has st_blksize.  */
#define HAVE_ST_BLKSIZE 1

/* Define if your struct stat has st_blocks.  */
#define HAVE_ST_BLOCKS 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if major, minor, and makedev are declared in <mkdev.h>.  */
/* #undef MAJOR_IN_MKDEV */

/* Define if major, minor, and makedev are declared in <sysmacros.h>.  */
/* #undef MAJOR_IN_SYSMACROS */

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
/* #undef STAT_MACROS_BROKEN */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define to a string giving the full name of the default archive file.  */
#define DEFAULT_ARCHIVE "-"

/* Define to a number giving the default blocking size for archives.  */
#define DEFAULT_BLOCKING 20

/* Define to 1 if density may be indicated by [lmh] at end of device.  */
/* #undef DENSITY_LETTER */

/* Define to a string giving the prefix of the default device, without the
   part specifying the unit and density.  */
/* #undef DEVICE_PREFIX */

/* Define to 1 if you lack a 3-argument version of open, and want to
   emulate it with system calls you do have.  */
/* #undef EMUL_OPEN3 */

/* Define to 1 if NLS is requested.  */
#define ENABLE_NLS 1

/* Define as 1 if you have catgets and don't want to use GNU gettext.  */
/* #undef HAVE_CATGETS */

/* Define to 1 if you have getgrgid(3).  */
#define HAVE_GETGRGID 1

/* Define to 1 if you have getpwuid(3).  */
#define HAVE_GETPWUID 1

/* Define as 1 if you have gettext and don't want to use GNU gettext.  */
#define HAVE_GETTEXT 1

/* Define if your locale.h file contains LC_MESSAGES.  */
#define HAVE_LC_MESSAGES 1

/* Define to 1 if you have the valloc function.  */
#define HAVE_VALLOC 1

/* Define to 1 if mknod function is available.  */
#define HAVE_MKNOD 1

 
/* Define if `union wait' is the type of the first arg to wait functions.  */
#define HAVE_UNION_WAIT 1

/* Define to 1 if utime.h exists and declares struct utimbuf.  */
#define HAVE_UTIME_H 1

/* Define to mt_model (v.g., for DG/UX), else to mt_type.  */
#define MTIO_CHECK_FIELD mt_type

/* Define to the name of the distribution.  */
#define PACKAGE "tar"

/* Define to 1 if ANSI function prototypes are usable.  */
#define PROTOTYPES 1

/* Define to the full path of your rsh, if any.  */
#define REMOTE_SHELL "/usr/bin/rsh"

/* Path to directory containing system wide message catalog sources.  */
#define STD_INC_PATH "/usr/local/share/nls/src"

/* Define to the version of the distribution.  */
#define VERSION "1.11.8"

/* Define to 1 for better use of the debugging malloc library.  See 
   site ftp.antaire.com in antaire/src, file dmalloc/dmalloc.tar.gz.  */
/* #undef WITH_DMALLOC */

/* Define to 1 if GNU regex should be used instead of GNU rx.  */
#define WITH_REGEX 1

/* The number of bytes in a unsigned int.  */
#define SIZEOF_UNSIGNED_INT 4

/* The number of bytes in a unsigned short.  */
#define SIZEOF_UNSIGNED_SHORT 2

/* Define if you have the ftime function.  */
#define HAVE_FTIME 1

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the isascii function.  */
#define HAVE_ISASCII 1

/* Define if you have the mkfifo function.  */
#define HAVE_MKFIFO 1

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the setenv function.  */
#define HAVE_SETENV 1

/* Define if you have the setlocale function.  */
#define HAVE_SETLOCALE 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the valloc function.  */
#define HAVE_VALLOC 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <locale.h> header file.  */
#define HAVE_LOCALE_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <netdb.h> header file.  */
/* #undef HAVE_NETDB_H */

/* Define if you have the <nl_types.h> header file.  */
#define HAVE_NL_TYPES_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/gentape.h> header file.  */
/* #undef HAVE_SYS_GENTAPE_H */

/* Define if you have the <sys/io/trioctl.h> header file.  */
/* #undef HAVE_SYS_IO_TRIOCTL_H */

/* Define if you have the <sys/ioccom.h> header file.  */
/* #undef HAVE_SYS_IOCCOM_H */

/* Define if you have the <sys/mtio.h> header file.  */
#define HAVE_SYS_MTIO_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/tape.h> header file.  */
/* #undef HAVE_SYS_TAPE_H */

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/timeb.h> header file.  */
#define HAVE_SYS_TIMEB_H 1

/* Define if you have the <sys/wait.h> header file.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <values.h> header file.  */
#define HAVE_VALUES_H 1

/* Define if you have the i library (-li).  */
/* #undef HAVE_LIBI */

/* Define if you have the intl library (-lintl).  */
/* #undef HAVE_LIBINTL */
