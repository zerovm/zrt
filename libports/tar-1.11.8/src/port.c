/* Supporting routines which may sometimes be missing.
   Copyright (C) 1988, 1992, 1994 Free Software Foundation, Inc.

   This file is part of GNU Tar.

   GNU Tar is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Tar is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Tar; see the file COPYING.  If not, write to
   the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "system.h"

#include <signal.h>

#define ISPRINT(Char) (ISASCII (Char) && isprint (Char))

extern long baserec;

/* All machine-dependent #ifdefs should appear here, instead of
   being scattered through the file.  For UN*X systems, it is better to
   figure out what is needed in the configure script, for most of the
   features. */

#ifdef __MSDOS__
char TTY_NAME[] = "con";
#define HAVE_STRSTR
#define HAVE_RENAME
#define HAVE_MKDIR
#else
char TTY_NAME[] = "/dev/tty";
#endif

/* End of system-dependent #ifdefs */

/* I'm not sure the following inclusion is useful...  */
#include "tar.h"

#ifdef minix

/*-------------------------------------------------------------------------.
| Groan, Minix doesn't have execlp either!				   |
| 									   |
| execlp(file,arg0,arg1...argn,(char *)NULL)				   |
| 									   |
| exec a program, automatically searching for the program through all the  |
| directories on the PATH.						   |
| 									   |
| This version is naive about variable argument lists, it assumes a	   |
| straightforward C calling sequence.  If your system has odd stacks *and* |
| doesn't have execlp, YOU get to fix it.				   |
`-------------------------------------------------------------------------*/

int
execlp (char *filename, char *arg0)
{
  register char *p, *path;
  register char *fnbuffer;
  char **argstart = &arg0;
  struct stat statbuf;
  extern char **environ;

  if (p = getenv ("PATH"), p == NULL)
    {

      /* Could not find path variable -- try to exec given filename.  */

      return execve (filename, argstart, environ);
    }

  /* Make a place to build the filename.  We malloc larger than we need,
     but we know it will fit in this.  */

  fnbuffer = malloc (strlen (p) + 1 + strlen (filename));
  if (fnbuffer == NULL)
    {
      errno = ENOMEM;
      return -1;
    }

  /* Try each component of the path to see if the file's there and
     executable.  */

  for (path = p; path; path = p)
    {

      /* Construct full path name to try.  */

      if (p = strchr (path, ':'), !p)
	strcpy (fnbuffer, path);
      else
	{
	  strncpy (fnbuffer, path, p - path);
	  fnbuffer[p - path] = '\0';
	  p++;			/* skip : for next time */
	}
      if (strlen (fnbuffer) != 0)
	strcat (fnbuffer, "/");
      strcat (fnbuffer, filename);

      /* Check to see if file is there and is a normal file.  */

      if (stat (fnbuffer, &statbuf) < 0)
	{
	  if (errno == ENOENT)
	    continue;		/* file not there,keep on looking */
	  else
	    goto fail;		/* failed for some reason, return */
	}
      if (!S_ISREG (statbuf.st_mode))
	continue;

      if (execve (fnbuffer, argstart, environ) < 0
	  && errno != ENOENT
	  && errno != ENOEXEC)
	{

	  /* Failed, for some other reason besides "file.  */

	  goto fail;
	}

      /* If we got error ENOEXEC, the file is executable but is not an
	 object file.  Try to execute it as a shell script, returning
	 error if we can't execute /bin/sh.

	 FIXME, this code is broken in several ways.  Shell scripts
	 should not in general be executed by the user's SHELL variable
	 program.  On more mature systems, the script can specify with
	 #!/bin/whatever.  Also, this code clobbers argstart[-1] if the
	 exec of the shell fails.  */

      if (errno == ENOEXEC)
	{
	  char *shell;

	  /* Try to execute command "sh arg0 arg1 ...".  */

	  if (shell = getenv ("SHELL"), shell == NULL)
	    shell = "/bin/sh";
	  argstart[-1] = shell;
	  argstart[0] = fnbuffer;
	  execve (shell, &argstart[-1], environ);
	  goto fail;		/* exec didn't work */
	}

      /* If we succeeded, the execve() doesn't return, so we can only be
	 here is if the file hasn't been found yet.  Try the next place
	 on the path.  */

    }

  /* All attempts failed to locate the file.  Give up.  */

  errno = ENOENT;

fail:
  free (fnbuffer);
  return -1;
}

#endif /* minix */

#ifdef EMUL_OPEN3
#include "open3.h"

/*-----------------------------------------------------------------------.
| open3 -- routine to emulate the 3-argument open system.		 |
| 									 |
| open3 (path, flag, mode);						 |
| 									 |
| Attempts to open the file specified by the given pathname.  The	 |
| following flag bits specify options to the routine.  Needless to say,	 |
| you should only specify one of the first three.  Function returns file |
| descriptor if successful, -1 and errno if not.			 |
`-----------------------------------------------------------------------*/

/* The following symbols should #defined in system.h...  Are they?  FIXME 

   O_RDONLY	file open for read only
   O_WRONLY	file open for write only
   O_RDWR	file open for both read & write

   O_CREAT	file is created with specified mode if it needs to be
   O_TRUNC	if file exists, it is truncated to 0 bytes
   O_EXCL	used with O_CREAT--routine returns error if file exists  */

/* Call that if present in most modern Unix systems.  This version
   attempts to support all the flag bits except for O_NDELAY and
   O_APPEND, which are silently ignored.  The emulation is not as
   efficient as the real thing (at worst, 4 system calls instead of one),
   but there's not much I can do about that.

   Written 6/10/87 by Richard Todd.  */

/* array to give arguments to access for various modes FIXME, this table
   depends on the specific integer values of O_*, and also contains
   integers (args to 'access') that should be #define's.  */

static int modes[] =
  {
    04,				/* O_RDONLY */
    02,				/* O_WRONLY */
    06,				/* O_RDWR */
    06,				/* invalid but we'd better cope -- O_WRONLY+O_RDWR */
  };

/* Shut off the automatic emulation of open(), we'll need it. */
#undef open

int
open3 (char *path, int flags, int mode)
{
  int exists = 1;
  int call_creat = 0;
  int fd;

  /* We actually do the work by calling the open() or creat() system
     call, depending on the flags.  Call_creat is true if we will use
     creat(), false if we will use open().  */

  /* See if the file exists and is accessible in the requested mode.

     Strictly speaking we shouldn't be using access, since access checks
     against real uid, and the open call should check against euid.  Most
     cases real uid == euid, so it won't matter.  FIXME.  FIXME, the
     construction "flags & 3" and the modes table depends on the specific
     integer values of the O_* #define's.  Foo!  */

  if (access (path, modes[flags & 3]) < 0)
    {
      if (errno == ENOENT)
	{

	  /* The file does not exist.  */

	  exists = 0;
	}
      else
	{

	  /* Probably permission violation.  */

	  if (flags & O_EXCL)
	    {

	      /* Oops, the file exists, we didn't want it.  No matter
		 what the error, claim EEXIST.  */

	      errno = EEXIST;
	    }
	  return -1;
	}
    }

  /* If we have the O_CREAT bit set, check for O_EXCL.  */

  if (flags & O_CREAT)
    {
      if ((flags & O_EXCL) && exists)
	{

	  /* Oops, the file exists and we didn't want it to.  */

	  errno = EEXIST;
	  return -1;
	}

      /* If the file doesn't exist, be sure to call creat() so that it
	 will be created with the proper mode.  */

      if (!exists)
	call_creat = 1;
    }
  else
    {

      /* If O_CREAT isn't set and the file doesn't exist, error.  */

      if (!exists)
	{
	  errno = ENOENT;
	  return -1;
	}
    }

  /* If the O_TRUNC flag is set and the file exists, we want to call
     creat() anyway, since creat() guarantees that the file will be
     truncated and open()-for-writing doesn't.  (If the file doesn't
     exist, we're calling creat() anyway and the file will be created
     with zero length.)  */

  if ((flags & O_TRUNC) && exists)
    call_creat = 1;

  /* Actually do the call.  */

  if (call_creat)
    {

      /* Call creat.  May have to close and reopen the file if we want
	 O_RDONLY or O_RDWR access -- creat() only gives O_WRONLY.  */

      fd = creat (path, mode);
      if (fd < 0 || (flags & O_WRONLY))
	return fd;
      if (close (fd) < 0)
	return -1;

      /* Fall out to reopen the file we've created.  */
    }

  /* Calling old open, we strip most of the new flags just in case.  */

  return open (path, flags & (O_RDONLY | O_WRONLY | O_RDWR | O_BINARY));
}

#endif /* EMUL_OPEN3 */

#ifndef HAVE_MKNOD
#ifdef __MSDOS__
typedef int dev_t;
#endif

/*----------------------------.
| Fake mknod by complaining.  |
`----------------------------*/

int
mknod (char *path, unsigned short mode, dev_t dev)
{
  int fd;

  errno = ENXIO;		/* no such device or address */
  return -1;			/* just give an error */
}

/*------------------------.
| Fake links by copying.  |
`------------------------*/

int
link (char *path1, char *path2)
{
  char buf[256];
  int ifd, ofd;
  int nrbytes;
  int nwbytes;

  WARN ((0, 0, _("%s: Cannot link to %s, copying instead"), path1, path2));
  if (ifd = open (path1, O_RDONLY | O_BINARY), ifd < 0)
    return -1;
  if (ofd = creat (path2, 0666), ofd < 0)
    return -1;
#ifdef __MSDOS__
  setmode (ofd, O_BINARY);
#endif
  while (nrbytes = read (ifd, buf, sizeof (buf)), nrbytes > 0)
    {
      if (nwbytes = write (ofd, buf, nrbytes), nwbytes != nrbytes)
	{
	  nrbytes = -1;
	  break;
	}
    }

  /* Note use of "|" rather than "||" below: we want to close.  */

  if ((nrbytes < 0) | (close (ifd) != 0) | (close (ofd) != 0))
    {
      unlink (path2);
      return -1;
    }
  return 0;
}

/* Everyone owns everything on MS-DOS (or is it no one owns anything?).  */

/*---.
| ?  |
`---*/

static int
chown (char *path, int uid, int gid)
{
  return 0;
}

/*---.
| ?  |
`---*/

int
geteuid (void)
{
  return 0;
}

#endif /* !HAVE_MKNOD */

#ifdef __TURBOC__
#include <time.h>
#include <fcntl.h>
#include <io.h>

struct utimbuf
  {
    time_t actime;		/* access time */
    time_t modtime;		/* modification time */
  };

/*---.
| ?  |
`---*/

int
utime (char *filename, struct utimbuf *utb)
{
  struct tm *tm;
  struct ftime filetime;
  time_t when;
  int fd;
  int status;

  if (utb == 0)
    when = time (0);
  else
    when = utb->modtime;

  fd = _open (filename, O_RDWR);
  if (fd == -1)
    return -1;

  tm = localtime (&when);
  if (tm->tm_year < 80)
    filetime.ft_year = 0;
  else
    filetime.ft_year = tm->tm_year - 80;
  filetime.ft_month = tm->tm_mon + 1;
  filetime.ft_day = tm->tm_mday;
  if (tm->tm_hour < 0)
    filetime.ft_hour = 0;
  else
    filetime.ft_hour = tm->tm_hour;
  filetime.ft_min = tm->tm_min;
  filetime.ft_tsec = tm->tm_sec / 2;

  status = setftime (fd, &filetime);
  _close (fd);
  return status;
}

#endif /* __TURBOC__ */

/*---.
| ?  |
`---*/

voidstar
ck_malloc (size_t size)
{
  if (!size)
    size++;
  return tar_xmalloc (size);
}

/* Implement a variable sized buffer of 'stuff'.  We don't know what it
   is, nor do we care, as long as it doesn't mind being aligned on a char
   boundry.  */

struct buffer
  {
    int allocated;
    int length;
    char *b;
  };

/*---.
| ?  |
`---*/

#define MIN_ALLOCATE 50

char *
init_buffer (void)
{
  struct buffer *b;

  b = (struct buffer *) tar_xmalloc (sizeof (struct buffer));
  b->allocated = MIN_ALLOCATE;
  b->b = (char *) tar_xmalloc (MIN_ALLOCATE);
  b->length = 0;
  return (char *) b;
}

/*---.
| ?  |
`---*/

void
flush_buffer (char *bb)
{
  struct buffer *b;

  b = (struct buffer *) bb;
  free (b->b);
  b->b = 0;
  b->allocated = 0;
  b->length = 0;
  free ((void *) b);
}

/*---.
| ?  |
`---*/

void
add_buffer (char *bb, const char *p, int n)
{
  struct buffer *b;

  b = (struct buffer *) bb;
  if (b->length + n > b->allocated)
    {
      b->allocated = b->length + n + MIN_ALLOCATE;
      b->b = (char *) tar_realloc (b->b, (size_t) b->allocated);
    }
  memcpy (b->b + b->length, p, (size_t) n);
  b->length += n;
}

/*---.
| ?  |
`---*/

char *
get_buffer (char *bb)
{
  struct buffer *b;

  b = (struct buffer *) bb;
  return b->b;
}

/*---.
| ?  |
`---*/

char *
merge_sort (char *list, unsigned n, int off, int (*cmp) ())
{
  char *ret;

  char *alist, *blist;
  unsigned alength, blength;

  char *tptr;
  int tmp;
  char **prev;
#define NEXTOF(Ptr)	(* ((char **)(((char *)(Ptr))+off) ) )
  if (n == 1)
    return list;
  if (n == 2)
    {
      if ((*cmp) (list, NEXTOF (list)) > 0)
	{
	  ret = NEXTOF (list);
	  NEXTOF (ret) = list;
	  NEXTOF (list) = 0;
	  return ret;
	}
      return list;
    }
  alist = list;
  alength = (n + 1) / 2;
  blength = n / 2;
  for (tptr = list, tmp = (n - 1) / 2; tmp; tptr = NEXTOF (tptr), tmp--)
    ;
  blist = NEXTOF (tptr);
  NEXTOF (tptr) = 0;

  alist = merge_sort (alist, alength, off, cmp);
  blist = merge_sort (blist, blength, off, cmp);
  prev = &ret;
  for (; alist && blist;)
    {
      if ((*cmp) (alist, blist) < 0)
	{
	  tptr = NEXTOF (alist);
	  *prev = alist;
	  prev = &(NEXTOF (alist));
	  alist = tptr;
	}
      else
	{
	  tptr = NEXTOF (blist);
	  *prev = blist;
	  prev = &(NEXTOF (blist));
	  blist = tptr;
	}
    }
  if (alist)
    *prev = alist;
  else
    *prev = blist;

  return ret;
}

/*---.
| ?  |
`---*/

void
ck_close (int fd)
{
  if (close (fd) < 0)
    ERROR ((TAREXIT_FAILURE, errno, _("Cannot close a file #%d"), fd));
}

#include <ctype.h>

/*-------------------------------------------------------------------------.
| Quote_copy_string is like quote_string, but instead of modifying the	   |
| string in place, it malloc-s a copy of the string, and returns that.  If |
| the string does not have to be quoted, it returns the NULL string.  The  |
| allocated copy can, of course, be freed with free() after the caller is  |
| done with it.								   |
`-------------------------------------------------------------------------*/

char *
quote_copy_string (const char *string)
{
  const char *from_here;
  char *to_there = NULL;
  char *copy_buf = NULL;
  int c;
  int copying = 0;

  from_here = string;
  while (*from_here)
    {
      c = (unsigned char) *from_here++;
      if (c == '\\')
	{
	  if (!copying)
	    {
	      int n;

	      n = (from_here - string) - 1;
	      copying++;
	      copy_buf = (char *) tar_xmalloc (n + 5 + strlen (from_here) * 4);
	      memcpy (copy_buf, string, (size_t) n);
	      to_there = copy_buf + n;
	    }
	  *to_there++ = '\\';
	  *to_there++ = '\\';
	}
      else if (ISPRINT (c))
	{
	  if (copying)
	    *to_there++ = c;
	}
      else
	{
	  if (!copying)
	    {
	      int n;

	      n = (from_here - string) - 1;
	      copying++;
	      copy_buf = (char *) tar_xmalloc (n + 5 + strlen (from_here) * 4);
	      memcpy (copy_buf, string, (size_t) n);
	      to_there = copy_buf + n;
	    }
	  *to_there++ = '\\';
	  if (c == '\n')
	    *to_there++ = 'n';
	  else if (c == '\t')
	    *to_there++ = 't';
	  else if (c == '\f')
	    *to_there++ = 'f';
	  else if (c == '\b')
	    *to_there++ = 'b';
	  else if (c == '\r')
	    *to_there++ = 'r';
	  else if (c == '\177')
	    *to_there++ = '?';
	  else
	    {
	      to_there[0] = (c >> 6) + '0';
	      to_there[1] = ((c >> 3) & 07) + '0';
	      to_there[2] = (c & 07) + '0';
	      to_there += 3;
	    }
	}
    }
  if (copying)
    {
      *to_there = '\0';
      return copy_buf;
    }
  return NULL;
}

/*-----------------------------------------------------------------------.
| Un_quote_string takes a quoted c-string (like those produced by	 |
| quote_string or quote_copy_string and turns it back into the un-quoted |
| original.  This is done in place.					 |
`-----------------------------------------------------------------------*/

/* There is no un-quote-copy-string.  Write it yourself */

char *
un_quote_string (char *string)
{
  char *ret;
  char *from_here;
  char *to_there;
  int tmp;

  ret = string;
  to_there = string;
  from_here = string;
  while (*from_here)
    {
      if (*from_here != '\\')
	{
	  if (from_here != to_there)
	    *to_there++ = *from_here++;
	  else
	    from_here++, to_there++;
	  continue;
	}
      switch (*++from_here)
	{
	case '\\':
	  *to_there++ = *from_here++;
	  break;
	case 'n':
	  *to_there++ = '\n';
	  from_here++;
	  break;
	case 't':
	  *to_there++ = '\t';
	  from_here++;
	  break;
	case 'f':
	  *to_there++ = '\f';
	  from_here++;
	  break;
	case 'b':
	  *to_there++ = '\b';
	  from_here++;
	  break;
	case 'r':
	  *to_there++ = '\r';
	  from_here++;
	  break;
	case '?':
	  *to_there++ = 0177;
	  from_here++;
	  break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	  tmp = *from_here - '0';
	  from_here++;
	  if (*from_here < '0' || *from_here > '7')
	    {
	      *to_there++ = tmp;
	      break;
	    }
	  tmp = tmp * 8 + *from_here - '0';
	  from_here++;
	  if (*from_here < '0' || *from_here > '7')
	    {
	      *to_there++ = tmp;
	      break;
	    }
	  tmp = tmp * 8 + *from_here - '0';
	  from_here++;
	  *to_there++ = tmp;
	  break;
	default:
	  ret = 0;
	  *to_there++ = '\\';
	  *to_there++ = *from_here++;
	  break;
	}
    }
  if (*to_there)
    *to_there++ = '\0';
  return ret;
}

#ifndef __MSDOS__

/*---.
| ?  |
`---*/

void
ck_pipe (int *pipes)
{
  if (pipe (pipes) < 0)
    ERROR ((TAREXIT_FAILURE, errno, _("Cannot open a pipe")));
}
#endif /* !__MSDOS__ */
