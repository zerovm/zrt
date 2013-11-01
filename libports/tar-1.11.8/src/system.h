/* System dependent definitions for GNU tar.
   Copyright (C) 1994 Free Software Foundation, Inc.
  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Declare alloca.  AIX requires this to be the first thing in the file.  */

#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca
char *alloca ();
#   endif
#  endif
# endif
#endif

#include <sys/types.h>

/* Declare a generic pointer type.  */
#if __STDC__ || defined(__TURBOC__)
# define voidstar void *
#else
# define voidstar char *
#endif

/* Declare ISASCII.  */

#include <ctype.h>

#if STDC_HEADERS
# define ISASCII(Char) 1
#else
# ifdef isascii
#  define ISASCII(Char) isascii (Char)
# else
#  if HAVE_ISASCII
#   define ISASCII(Char) isascii (Char)
#  else
#   define ISASCII(Char) 1
#  endif
# endif
#endif

/* Declare string and memory handling routines.  Take care that an ANSI
   string.h and pre-ANSI memory.h might conflict, and that memory.h and
   strings.h conflict on some systems.  */

#if STDC_HEADERS || HAVE_STRING_H
# include <string.h>
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
#else
# include <strings.h>
# ifndef strchr
#  define strchr index
# endif
# ifndef strrchr
#  define strrchr rindex
# endif
# ifndef memcpy
#  define memcpy(Dst, Src, Num) bcopy (Src, Dst, Num)
# endif
# ifndef memcmp
#  define memcmp(Src1, Src2, Num) bcmp (Src1, Src2, Num)
# endif
#endif

/* Declare errno.  */

#include <errno.h>
#ifndef errno
extern int errno;
#endif

/* Declare open parameters.  */

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# include <sys/file.h>
#endif

#ifndef	O_BINARY
# define O_BINARY 0
#endif
#ifndef O_CREAT
# define O_CREAT 0
#endif
#ifndef	O_NDELAY
# define O_NDELAY 0
#endif
#ifndef	O_RDONLY
# define O_RDONLY 0
#endif
#ifndef O_RDWR
# define O_RDWR 2
#endif

/* Declare file status routines and bits.  */

#include <sys/stat.h>

#ifdef STAT_MACROS_BROKEN
# undef S_ISBLK
# undef S_ISCHR
# undef S_ISDIR
# undef S_ISFIFO
# undef S_ISLNK
# undef S_ISMPB
# undef S_ISMPC
# undef S_ISNWK
# undef S_ISREG
# undef S_ISSOCK
#endif

/* On MSDOS, there are missing things from <sys/stat.h>.  */
#ifdef __MSDOS__
# define S_ISUID 0
# define S_ISGID 0
# define S_ISVTX 0
#endif

#ifndef S_ISREG			/* POSIX.1 stat stuff missing */
# define mode_t unsigned short
#endif
#if !defined(S_ISBLK) && defined(S_IFBLK)
# define S_ISBLK(Mode) (((Mode) & S_IFMT) == S_IFBLK)
#endif
#if !defined(S_ISCHR) && defined(S_IFCHR)
# define S_ISCHR(Mode) (((Mode) & S_IFMT) == S_IFCHR)
#endif
#if !defined(S_ISDIR) && defined(S_IFDIR)
# define S_ISDIR(Mode) (((Mode) & S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISREG) && defined(S_IFREG)
# define S_ISREG(Mode) (((Mode) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISFIFO) && defined(S_IFIFO)
# define S_ISFIFO(Mode) (((Mode) & S_IFMT) == S_IFIFO)
#endif
#if !defined(S_ISLNK) && defined(S_IFLNK)
# define S_ISLNK(Mode) (((Mode) & S_IFMT) == S_IFLNK)
#endif
#if !defined(S_ISSOCK) && defined(S_IFSOCK)
# define S_ISSOCK(Mode) (((Mode) & S_IFMT) == S_IFSOCK)
#endif
#if !defined(S_ISMPB) && defined(S_IFMPB) /* V7 */
# define S_ISMPB(Mode) (((Mode) & S_IFMT) == S_IFMPB)
# define S_ISMPC(Mode) (((Mode) & S_IFMT) == S_IFMPC)
#endif
#if !defined(S_ISNWK) && defined(S_IFNWK) /* HP/UX */
# define S_ISNWK(Mode) (((Mode) & S_IFMT) == S_IFNWK)
#endif

#ifndef HAVE_MKFIFO
# define mkfifo(Path, Mode) (mknod (Path, (Mode) | S_IFIFO, 0))
#endif

#if !defined(S_ISCTG) && defined(S_IFCTG) /* contiguous file */
# define S_ISCTG(Mode) (((Mode) & S_IFMT) == S_IFCTG)
#endif
#if !defined(S_ISVTX)
# define S_ISVTX 0001000
#endif

#ifndef _POSIX_SOURCE
# include <sys/param.h>
#endif

/* Include <unistd.h> before any preprocessor test of _POSIX_VERSION.  */
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/* Declare make device, major and minor.  Since major is a function on
   SVR4, we have to resort to GOT_MAJOR instead of just testing if
   major is #define'd.  */

#ifdef MAJOR_IN_MKDEV
# include <sys/mkdev.h>
# define GOT_MAJOR
#endif

#ifdef MAJOR_IN_SYSMACROS
# include <sys/sysmacros.h>
# define GOT_MAJOR
#endif

/* Some <sys/types.h> defines the macros. */
#ifdef major
# define GOT_MAJOR
#endif

#ifndef GOT_MAJOR
# ifdef __MSDOS__
#  define major(Device)		(Device)
#  define minor(Device)		(Device)
#  define makedev(Major, Minor)	(((Major) << 8) | (Minor))
#  define GOT_MAJOR
# endif
#endif

/* For HP-UX before HP-UX 8, major/minor are not in <sys/sysmacros.h>.  */
#ifndef GOT_MAJOR
# if defined(hpux) || defined(__hpux__) || defined(__hpux)
#  include <sys/mknod.h>
#  define GOT_MAJOR
# endif
#endif

#ifndef GOT_MAJOR
# define major(Device)		(((Device) >> 8) & 0xff)
# define minor(Device)		((Device) & 0xff)
# define makedev(Major, Minor)	(((Major) << 8) | (Minor))
#endif

#undef GOT_MAJOR

/* Declare directory reading routines and structures.  */

#ifdef __MSDOS__
# include "msd_dir.h"
# define NAMLEN(dirent) ((dirent)->d_namlen)
#else
# ifdef HAVE_DIRENT_H
#  include <dirent.h>
#  define NAMLEN(dirent) (strlen((dirent)->d_name))
# else
#  define dirent direct
#  define NAMLEN(dirent) ((dirent)->d_namlen)
#  ifdef HAVE_SYS_NDIR_H
#   include <sys/ndir.h>
#  endif
#  ifdef HAVE_SYS_DIR_H
#   include <sys/dir.h>
#  endif
#  ifdef HAVE_NDIR_H
#   include <ndir.h>
#  endif
# endif
#endif

/* Declare wait status.  */

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_UNION_WAIT
# define WAIT_T union wait
# ifndef WTERMSIG
#  define WTERMSIG(Status)     ((Status).w_termsig)
# endif
# ifndef WCOREDUMP
#  define WCOREDUMP(Status)    ((Status).w_coredump)
# endif
# ifndef WEXITSTATUS
#  define WEXITSTATUS(Status)  ((Status).w_retcode)
# endif
#else
# define WAIT_T int
# ifndef WTERMSIG
#  define WTERMSIG(Status)     ((Status) & 0x7f)
# endif
# ifndef WCOREDUMP
#  define WCOREDUMP(Status)    ((Status) & 0x80)
# endif
# ifndef WEXITSTATUS
#  define WEXITSTATUS(Status)  (((Status) >> 8) & 0xff)
# endif
#endif

#ifndef WIFSTOPPED
# define WIFSTOPPED(Status)    (WTERMSIG(Status) == 0x7f)
#endif
#ifndef WIFSIGNALED
# define WIFSIGNALED(Status)   (WTERMSIG(Status) != 0)
#endif
#ifndef WIFEXITED
# define WIFEXITED(Status)     (WTERMSIG(Status) == 0)
#endif
	 
/* It is wrong to use RECORDSIZE for buffers when the logical block size
   is greater than 512 bytes; so ST_BLKSIZE code below, in preparation
   for some cleanup in this area, later.  FIXME.  */

/* Get or fake the disk device blocksize.  Usually defined by sys/param.h
   (if at all).  */

#if !defined(DEV_BSIZE) && defined(BSIZE)
# define DEV_BSIZE BSIZE
#endif
#if !defined(DEV_BSIZE) && defined(BBSIZE) /* SGI */
# define DEV_BSIZE BBSIZE
#endif
#ifndef DEV_BSIZE
# define DEV_BSIZE 4096
#endif

/* Extract or fake data from a `struct stat'.  ST_BLKSIZE gives the
   optimal I/O blocksize for the file, in bytes.  Some systems, like
   Sequents, return st_blksize of 0 on pipes.  */

#ifndef HAVE_ST_BLKSIZE
# define ST_BLKSIZE(Statbuf) DEV_BSIZE
#else
# define ST_BLKSIZE(Statbuf) \
    ((Statbuf).st_blksize > 0 ? (Statbuf).st_blksize : DEV_BSIZE)
#endif

/* Extract or fake data from a `struct stat'.  ST_NBLOCKS gives the
   number of 512-byte blocks in the file (including indirect blocks).
   fileblocks.c uses BSIZE.  HP-UX counts st_blocks in 1024-byte units,
   this loses when mixing HP-UX and BSD filesystems with NFS.  AIX PS/2
   counts st_blocks in 4K units.  */

#ifndef HAVE_ST_BLOCKS
# if defined(_POSIX_SOURCE) || !defined(BSIZE)
#  define ST_NBLOCKS(Statbuf) (((Statbuf).st_size + 512 - 1) / 512)
# else
#  define ST_NBLOCKS(Statbuf) (st_blocks ((Statbuf).st_size))
# endif
#else
# if defined(hpux) || defined(__hpux__) || defined(__hpux)
#  define ST_NBLOCKS(Statbuf) ((Statbuf).st_blocks * 2)
# else
#  if defined(_AIX) && defined(_I386)
#   define ST_NBLOCKS(Statbuf) ((Statbuf).st_blocks * 8)
#  else
#   define ST_NBLOCKS(Statbuf) ((Statbuf).st_blocks)
#  endif
# endif
#endif

#if HAVE_SYS_GENTAPE_H		/* e.g., for ISC */
# include <sys/gentape.h>
#else
# if HAVE_SYS_TAPE_H		/* e.g., for SCO */
#  include <sys/tape.h>
# else
#  if HAVE_SYS_MTIO_H
#   include <sys/ioctl.h>
#   if HAVE_SYS_IO_TRIOCTL_H
#    include <sys/io/trioctl.h>
#   endif
#   include <sys/mtio.h>
#  endif
# endif
#endif

/* Declare standard functions.  */

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
voidstar malloc ();
voidstar realloc ();
# ifdef HAVE_GETCWD
char *getcwd ();
# endif
char *getenv ();
#endif

#include <stdio.h>

#ifndef _POSIX_VERSION
# ifdef __MSDOS__
#  include <io.h>
# else
off_t lseek ();
# endif
#endif

#include <pathmax.h>

/* Until getoptold function is declared in getopt.h, we need this here for
   struct option.  */
#include <getopt.h>

#ifdef WITH_DMALLOC
# undef HAVE_VALLOC
# define DMALLOC_FUNC_CHECK
# include <dmalloc.h>
#endif

/* Prototypes for external functions.  */

#ifndef __P
# if PROTOTYPES
#  define __P(Args) Args
# else
#  define __P(Args) ()
# endif
#endif

#if HAVE_LOCALE_H
# include <locale.h>
#endif
#if !HAVE_SETLOCALE
# define setlocale(Category, Locale)
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define textdomain(Domain)
# define _(Text) Text
#endif

/* Library modules.  */

#ifdef HAVE_VPRINTF
void error __P ((int, int, const char *, ...));
#else
void error ();
#endif

#ifndef HAVE_STRSTR
char *strstr __P ((const char *, const char *));
#endif

#ifndef HAVE_VALLOC
# define valloc(Size) malloc (Size)
#endif

voidstar xmalloc __P ((size_t));
voidstar tar_realloc __P ((voidstar, size_t));
char *xstrdup __P ((const char *));
