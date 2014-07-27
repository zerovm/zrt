/* Functions for communicating with a remote tape drive.
   Copyright (C) 1988, 1992, 1994 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* The man page rmt(8) for /etc/rmt documents the remote mag tape
   protocol which rdump and rrestore use.  Unfortunately, the man page is
   *WRONG*.  The author of the routines I'm including originally wrote
   his code just based on the man page, and it didn't work, so he went to
   the rdump source to figure out why.  The only thing he had to change
   was to check for the 'F' return code in addition to the 'E', and to
   separate the various arguments with \n instead of a space.  I
   personally don't think that this is much of a problem, but I wanted to
   point it out. -- Arnold Robbins
   
   Originally written by Jeff Lee, modified some by Arnold Robbins.
   Redone as a library that can replace open, read, write, etc., by Fred
   Fish, with some additional work by Arnold Robbins.  Modified to make
   all rmt* calls into macros for speed by Jay Fenlason.  Use
   -DHAVE_NETDB_H for rexec code, courtesy of Dan Kegel.  */

#include "system.h"

#include <signal.h>

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#include <setjmp.h>

#include "rmt.h"

/* Just to shut up -Wall.  FIXME.  */
int rexec ();

/* Exit status if exec errors.  */
#define EXIT_ON_EXEC_ERROR 128

/* Size of buffers for reading and writing commands to rmt.  FIXME.  */
#define CMDBUFSIZE 64

#ifndef RETSIGTYPE
#define RETSIGTYPE void
#endif

/* Maximum number of simultaneous remote tape connections.  FIXME.  */
#define MAXUNIT	4

/* Return the parent's read side of remote tape connection Fd.  */
#define READ(Fd) (from_remote[Fd][0])

/* Return the parent's write side of remote tape connection Fd.  */
#define WRITE(Fd) (to_remote[Fd][1])

/* The pipes for receiving data from remote tape drives.  */
static int from_remote[MAXUNIT][2] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}};

/* The pipes for sending data to remote tape drives.  */
static int to_remote[MAXUNIT][2] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}};

/* Temporary variable used by macros in rmt.h.  */
char *__rmt_path;


/*-----------------------------------------------.
| Close remote tape connection FILE_DESCRIPTOR.	 |
`-----------------------------------------------*/

static void
__tar_rmt_shutdown (int file_descriptor)
{
  close (READ (file_descriptor));
  close (WRITE (file_descriptor));
  READ (file_descriptor) = -1;
  WRITE (file_descriptor) = -1;
}

/*------------------------------------------------------------------------.
| Attempt to perform the remote tape command specified in BUF on remote	  |
| tape connection FILE_DESCRIPTOR.  Return 0 if successful, -1 on error.  |
`------------------------------------------------------------------------*/

static int
__tar_do_command (int file_descriptor, const char *buf)
{
  register int buflen;
  RETSIGTYPE (*pipe_handler) ();

  /* Save the current pipe handler and try to make the request.  */

  pipe_handler = signal (SIGPIPE, SIG_IGN);
  buflen = strlen (buf);
  if (write (WRITE (file_descriptor), buf, (size_t) buflen) == buflen)
    {
      signal (SIGPIPE, pipe_handler);
      return 0;
    }

  /* Something went wrong.  Close down and go home.  */

  signal (SIGPIPE, pipe_handler);
  __tar_rmt_shutdown (file_descriptor);
  errno = EIO;
  return -1;
}

/*-------------------------------------------------------------------------.
| Read and return the status from remote tape connection FILE_DESCRIPTOR.  |
| If an error occurred, return -1 and set errno.			   |
`-------------------------------------------------------------------------*/

static int
__tar_get_status (int file_descriptor)
{
  int i;
  char c, *cp;
  char command_buffer[CMDBUFSIZE];

  /* Read the reply command line.  */

  for (i = 0, cp = command_buffer; i < CMDBUFSIZE; i++, cp++)
    {
      if (read (READ (file_descriptor), cp, 1) != 1)
	{
	  __tar_rmt_shutdown (file_descriptor);
	  errno = EIO;
	  return -1;
	}
      if (*cp == '\n')
	{
	  *cp = '\0';
	  break;
	}
    }

  if (i == CMDBUFSIZE)
    {
      __tar_rmt_shutdown (file_descriptor);
      errno = EIO;
      return -1;
    }

  /* Check the return status.  */

  for (cp = command_buffer; *cp; cp++)
    if (*cp != ' ')
      break;

  if (*cp == 'E' || *cp == 'F')
    {
      errno = atoi (cp + 1);

      /* Skip the error message line.  */

      while (read (READ (file_descriptor), &c, 1) == 1)
	if (c == '\n')
	  break;

      if (*cp == 'F')
	__tar_rmt_shutdown (file_descriptor);

      return -1;
    }

  /* Check for mis-synced pipes.  */

  if (*cp != 'A')
    {
      __tar_rmt_shutdown (file_descriptor);
      errno = EIO;
      return -1;
    }

  /* Got an `A' (success) response.  */

  return atoi (cp + 1);
}

#ifdef HAVE_NETDB_H

/*-------------------------------------------------------------------------.
| Execute /etc/rmt as user USER on remote system HOST using rexec.  Return |
| a file descriptor of a bidirectional socket for stdin and stdout.  If	   |
| USER is NULL, use the current username.				   |
| 									   |
| By default, this code is not used, since it requires that the user have  |
| a .netrc file in his/her home directory, or that the application	   |
| designer be willing to have rexec prompt for login and password info.	   |
| This may be unacceptable, and .rhosts files for use with rsh are much	   |
| more common on BSD systems.						   |
`-------------------------------------------------------------------------*/

static int
_rmt_rexec (char *host, char *user)
{
  struct servent *rexecserv;
  int save_stdin = dup (fileno (stdin));
  int save_stdout = dup (fileno (stdout));
  int tape_fd;			/* return value */

  /* When using cpio -o < filename, stdin is no longer the tty.  But the
     rexec subroutine reads the login and the passwd on stdin, to allow
     remote execution of the command.  So, reopen stdin and stdout on
     /dev/tty before the rexec and give them back their original value
     after.  */

  if (freopen ("/dev/tty", "r", stdin) == NULL)
    freopen ("/dev/null", "r", stdin);
  if (freopen ("/dev/tty", "w", stdout) == NULL)
    freopen ("/dev/null", "w", stdout);

  if (rexecserv = getservbyname ("exec", "tcp"), !rexecserv)
    error (EXIT_ON_EXEC_ERROR, 0, _("exec/tcp: Service not available"));

  tape_fd = rexec (&host, rexecserv->s_port, user, NULL,
		   "/etc/rmt", (int *) NULL);
  fclose (stdin);
  fdopen (save_stdin, "r");
  fclose (stdout);
  fdopen (save_stdout, "w");

  return tape_fd;
}

#endif /* HAVE_NETDB_H */

/*-------------------------------------------------------------------------.
| Open a file (some magnetic tape device?) on the system specified in	   |
| PATH, as the given user.  PATH has the form `[USER@]HOST:FILE'.  OFLAG   |
| is O_RDONLY, O_WRONLY, etc.  If successful, return the remote pipe	   |
| number plus BIAS.  REMOTE_SHELL may be overriden.  On error, return -1.  |
`-------------------------------------------------------------------------*/

#ifdef __ZRT__
int
__tar_rmt_open (const char *path, int oflag, int bias, const char *remote_shell)
{
    return -1;
}
#else
int
__tar_rmt_open (const char *path, int oflag, int bias, const char *remote_shell)
{
  int remote_pipe_number;	/* pseudo, biased file descriptor */
  char *path_copy;		/* copy of path string */
  char *cursor;			/* cursor in path_copy */
  char *remote_host;		/* remote host name */
  char *remote_file;		/* remote file name (often a device) */
  char *remote_user;		/* remote user name */
  char command_buffer[CMDBUFSIZE];

#ifndef HAVE_NETDB_H
  const char *remote_shell_basename;
  int status;
#endif

  /* Find an unused pair of file descriptors.  */

  for (remote_pipe_number = 0;
       remote_pipe_number < MAXUNIT;
       remote_pipe_number++)
    if (READ (remote_pipe_number) == -1
	&& WRITE (remote_pipe_number) == -1)
      break;

  if (remote_pipe_number == MAXUNIT)
    {
      errno = EMFILE;
      return -1;
    }

  /* Pull apart the system and device, and optional user.  */

  path_copy = __tar_xstrdup (path);
  remote_host = path_copy;
  remote_user = NULL;
  remote_file = NULL;

  for (cursor = path_copy; *cursor; cursor++)
    switch (*cursor)
      {
      default:
	break;

      case '@':
	if (!remote_user)
	  {
	    remote_user = remote_host;
	    *cursor = '\0';
	    remote_host = cursor + 1;
	  }
	break;

      case ':':
	if (!remote_file)
	  {
	    *cursor = '\0';
	    remote_file = cursor + 1;
	  }
	break;
      }

  /* FIXME: Should somewhat validate the decoding, here.  */

  if (*remote_user == '\0')
    remote_user = NULL;

#ifdef HAVE_NETDB_H

  /* Execute the remote command using rexec.  */

  READ (remote_pipe_number) = _rmt_rexec (remote_host, remote_user);
  if (READ (remote_pipe_number) < 0)
    {
      free (path_copy);
      return -1;
    }

  WRITE (remote_pipe_number) = READ (remote_pipe_number);

#else /* not HAVE_NETDB_H */

  /* Identify the remote command to be executed.  */

  if (!remote_shell)
    {
#ifdef REMOTE_SHELL
      remote_shell = REMOTE_SHELL;
#else
      errno = EIO;
      free (path_copy);
      return -1;
#endif
    }
  remote_shell_basename = strrchr (remote_shell, '/');
  if (remote_shell_basename)
    remote_shell_basename++;
  else
    remote_shell_basename = remote_shell;

  /* Set up the pipes for the `rsh' command, and fork.  */

  if (pipe (to_remote[remote_pipe_number]) == -1
      || pipe (from_remote[remote_pipe_number]) == -1)
    {
      free (path_copy);
      return -1;
    }

  status = fork ();
  if (status == -1)
    {
      free (path_copy);
      return -1;
    }

  if (status == 0)
    {

      /* Child.  */

      close (0);
      dup (to_remote[remote_pipe_number][0]);
      close (to_remote[remote_pipe_number][0]);
      close (to_remote[remote_pipe_number][1]);

      close (1);
      dup (from_remote[remote_pipe_number][1]);
      close (from_remote[remote_pipe_number][0]);
      close (from_remote[remote_pipe_number][1]);

      setuid (getuid ());
      setgid (getgid ());

      if (remote_user)
	execl (remote_shell, remote_shell_basename, remote_host,
	       "-l", remote_user, "/etc/rmt", (char *) 0);
      else
	execl (remote_shell, remote_shell_basename, remote_host,
	       "/etc/rmt", (char *) 0);

      /* Bad problems if we get here.  */
      
      /* In a previous version, _exit was used here instead of exit.  */
      error (EXIT_ON_EXEC_ERROR, errno, _("Cannot execute remote shell"));
    }

  /* Parent.  */

  close (from_remote[remote_pipe_number][1]);
  close (to_remote[remote_pipe_number][0]);

#endif /* not HAVE_NETDB_H */

  /* Attempt to open the tape device.  */

  sprintf (command_buffer, "O%s\n%d\n", remote_file, oflag);
  if (__tar_do_command (remote_pipe_number, command_buffer) == -1
      || __tar_get_status (remote_pipe_number) == -1)
    {
      free (path_copy);
      return -1;
    }

  free (path_copy);
  return remote_pipe_number + bias;
}
#endif //__ZRT__

/*-------------------------------------------------------------------------.
| Close remote tape connection FILE_DESCRIPTOR and shut down.  Return 0 if |
| successful, -1 on error.						   |
`-------------------------------------------------------------------------*/

int
__tar_rmt_close (int file_descriptor)
{
  int status;

  if (__tar_do_command (file_descriptor, "C\n") == -1)
    return -1;

  status = __tar_get_status (file_descriptor);
  __tar_rmt_shutdown (file_descriptor);
  return status;
}

/*--------------------------------------------------------------------.
| Read up to NBYTE bytes into BUF from remote tape connection	      |
| FILE_DESCRIPTOR.  Return the number of bytes read on success, -1 on |
| error.							      |
`--------------------------------------------------------------------*/

int
__tar_rmt_read (int file_descriptor, char *buf, unsigned int nbyte)
{
  int status, i;
  char command_buffer[CMDBUFSIZE];

  sprintf (command_buffer, "R%d\n", nbyte);
  if (__tar_do_command (file_descriptor, command_buffer) == -1
      || (status = __tar_get_status (file_descriptor)) == -1)
    return -1;

  for (i = 0; i < status; i += nbyte, buf += nbyte)
    {
      nbyte = read (READ (file_descriptor), buf, (size_t) (status - i));
      if (nbyte <= 0)
	{
	  __tar_rmt_shutdown (file_descriptor);
	  errno = EIO;
	  return -1;
	}
    }

  return status;
}

/*-----------------------------------------------------------------------.
| Write NBYTE bytes from BUF to remote tape connection FILE_DESCRIPTOR.	 |
| Return the number of bytes written on success, -1 on error.		 |
`-----------------------------------------------------------------------*/

int
__tar_rmt_write (int file_descriptor, char *buf, unsigned int nbyte)
{
  char command_buffer[CMDBUFSIZE];
  RETSIGTYPE (*pipe_handler) ();

  sprintf (command_buffer, "W%d\n", nbyte);
  if (__tar_do_command (file_descriptor, command_buffer) == -1)
    return -1;

  pipe_handler = signal (SIGPIPE, SIG_IGN);
  if (write (WRITE (file_descriptor), buf, nbyte) == nbyte)
    {
      signal (SIGPIPE, pipe_handler);
      return __tar_get_status (file_descriptor);
    }

  /* Write error.  */

  signal (SIGPIPE, pipe_handler);
  __tar_rmt_shutdown (file_descriptor);
  errno = EIO;
  return -1;
}

/*---------------------------------------------------------------------.
| Perform an imitation lseek operation on remote tape connection       |
| FILE_DESCRIPTOR.  Return the new file offset if successful, -1 if on |
| error.							       |
`---------------------------------------------------------------------*/

long
__tar_rmt_lseek (int file_descriptor, off_t offset, int whence)
{
  char command_buffer[CMDBUFSIZE];

  sprintf (command_buffer, "L%ld\n%d\n", offset, whence);
  if (__tar_do_command (file_descriptor, command_buffer) == -1)
    return -1;

  return __tar_get_status (file_descriptor);
}

/*-------------------------------------------------------------------------.
| Perform a raw tape operation on remote tape connection FILE_DESCRIPTOR.  |
| Return the results of the ioctl, or -1 on error.			   |
`-------------------------------------------------------------------------*/

int
__tar_rmt_ioctl (int file_descriptor, int op, char *arg)
{
  char c;
  int status, cnt;
  char command_buffer[CMDBUFSIZE];

  switch (op)
    {
    default:
      errno = EOPNOTSUPP;
      return -1;

#ifdef MTIOCTOP

    case MTIOCTOP:

      /* MTIOCTOP is the easy one.  Nothing is transfered in binary.  */

      sprintf (command_buffer, "I%d\n%d\n", ((struct mtop *) arg)->mt_op,
	       ((struct mtop *) arg)->mt_count);
      if (__tar_do_command (file_descriptor, command_buffer) == -1)
	return -1;

      /* Return the count.  */

      return __tar_get_status (file_descriptor);

#endif /* MTIOCTOP */

#ifdef MTIOCGET

    case MTIOCGET:

      /* Grab the status and read it directly into the structure.  This
	 assumes that the status buffer is not padded and that 2 shorts
	 fit in a long without any word alignment problems; i.e., the
	 whole struct is contiguous.  NOTE - this is probably NOT a good
	 assumption.  */

      if (__tar_do_command (file_descriptor, "S") == -1
	  || (status = __tar_get_status (file_descriptor), status == -1))
	return -1;

      for (; status > 0; status -= cnt, arg += cnt)
	{
	  cnt = read (READ (file_descriptor), arg, (size_t) status);
	  if (cnt <= 0)
	    {
	      __tar_rmt_shutdown (file_descriptor);
	      errno = EIO;
	      return -1;
	    }
	}

      /* Check for byte position.  mt_type (or mt_model) is a small integer
	 field (normally) so we will check its magnitude.  If it is larger
	 than 256, we will assume that the bytes are swapped and go through
	 and reverse all the bytes.  */

      if (((struct mtget *) arg)->MTIO_CHECK_FIELD < 256)
	return 0;

      for (cnt = 0; cnt < status; cnt += 2)
	{
	  c = arg[cnt];
	  arg[cnt] = arg[cnt + 1];
	  arg[cnt + 1] = c;
	}

      return 0;

#endif /* MTIOCGET */

    }
}
