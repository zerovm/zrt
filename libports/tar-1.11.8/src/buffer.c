/* Buffer management for tar.
   Copyright (C) 1988, 1992, 1993, 1994 Free Software Foundation, Inc.

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

/* Buffer management for tar.
   Written by John Gilmore, on 25 August 1985.  */

#include "system.h"

#include <signal.h>
#include <time.h>
time_t time ();

#ifdef	__MSDOS__
#include <process.h>
#endif

#ifdef XENIX
#include <sys/inode.h>
#endif

#if WITH_REGEX
# include <regex.h>
#else
# include <rx.h>
#endif

#include "rmt.h"
#include "tar.h"

/* Where we write messages (standard messages, not errors) to.  Stdout
   unless we're writing a pipe, in which case stderr.  */
FILE *stdlis;

#define	STDIN	0		/* standard input  file descriptor */
#define	STDOUT	1		/* standard output file descriptor */

#define	PREAD	0		/* read  file descriptor from pipe() */
#define	PWRITE	1		/* write file descriptor from pipe() */

static int __tar_backspace_output __P ((void));
static int __tar_new_volume __P ((int));
static void __tar_writeerror __P ((int));
static void __tar_readerror __P ((void));

#ifndef __MSDOS__
/* Obnoxious test to see if dimwit is trying to dump the archive */
dev_t ar_dev;
ino_t ar_ino;
#endif

/* The record pointed to by save_rec should not be overlaid when reading
   in a new tape block.  Copy it to record_save_area first, and change
   the pointer in *save_rec to point to record_save_area.  Saved_recno
   records the record number at the time of the save.  This is used by
   annofile() to print the record number of a file's header record.  */
static union record **save_rec;
union record record_save_area;
static long saved_recno;

/* PID of child program, if flag_compress or remote archive access.  */
static int childpid = 0;

/* Record number of the start of this block of records  */
long baserec;

/* Error recovery stuff  */
static int r_error_count;

/* Have we hit EOF yet?  */
static int hit_eof;

/* Checkpointing counter */
static int checkpoint;

/* JF we're reading, but we just read the last record and its time to update */
extern time_to_start_writing;
int file_to_switch_to = -1;	/* if remote update, close archive, and use
				   this descriptor to write to */

static int volno = 1;		/* JF which volume of a multi-volume tape
				   we're on */
static int global_volno = 1;	/* volume number to print in external messages */

char *save_name;		/* name of the file we are currently writing */
long save_totsize;		/* total size of file we are writing, only
				   valid if save_name is non NULL */
long save_sizeleft;		/* where we are in the file we are writing,
				   only valid if save_name is non-zero */

int write_archive_to_stdout;

/* Used by __tar_fl_read and __tar_fl_write to store the real info about saved names */
static char real_s_name[NAMSIZ];
static long real_s_totsize;
static long real_s_sizeleft;

/*---------------------------------------------------------.
| Reset the EOF flag (if set), and re-set ar_record, etc.  |
`---------------------------------------------------------*/

void
__tar_reset_eof (void)
{
  if (hit_eof)
    {
      hit_eof = 0;
      ar_record = ar_block;
      ar_last = ar_block + blocking;
      ar_reading = 0;
    }
}

/*-------------------------------------------------------------------------.
| Return the location of the next available input or output record.	   |
| Return NULL for EOF.  Once we have returned NULL, we just keep returning |
| it, to avoid accidentally going on to the next file on the "tape".	   |
`-------------------------------------------------------------------------*/

union record *
__tar_findrec (void)
{
  if (ar_record == ar_last)
    {
      if (hit_eof)
	return NULL;
      __tar_flush_archive ();
      if (ar_record == ar_last)
	{
	  hit_eof++;
	  return NULL;
	}
    }
  return ar_record;
}

/*----------------------------------------------------------------------.
| Indicate that we have used all records up thru the argument.  (should |
| the arg have an off-by-1?  FIXME)				        |
`----------------------------------------------------------------------*/

void
__tar_userec (union record *rec)
{
  while (rec >= ar_record)
    ar_record++;

  /* Do *not* flush the archive here.  If we do, the same argument to
     __tar_userec() could mean the next record (if the input block is exactly
     one record long), which is not what is intended.  */

  if (ar_record > ar_last)
    abort ();
}

/*----------------------------------------------------------------------.
| Return a pointer to the end of the current records buffer.  All the   |
| space between __tar_findrec() and __tar_endofrecs() is available for filling with |
| data, or taking data from.					        |
`----------------------------------------------------------------------*/

union record *
__tar_endofrecs (void)
{
  return ar_last;
}

/*--------------------------------------------------------------------.
| Duplicate a file descriptor into a certain slot.  Equivalent to BSD |
| "dup2" with error reporting.					      |
`--------------------------------------------------------------------*/

static void
__tar_dupto (int from, int to, const char *msg)
{
  int err;

  if (from != to)
    {
      err = close (to);
      if (err < 0 && errno != EBADF)
	ERROR ((TAREXIT_FAILURE, errno, _("Cannot close descriptor %d"), to));
      err = dup (from);
      if (err != to)
	ERROR ((TAREXIT_FAILURE, errno, _("Cannot dup %s"), msg));
      __tar_ck_close (from);
    }
}

/*---.
| ?  |
`---*/

#if defined(__native_client__) || defined(__MSDOS__)
static void
__tar_child_open (void)
{
    ERROR ((TAREXIT_FAILURE, 0, _("Cannot use compressed or remote archives")));
}

#else
static void
__tar_child_open (void)
{
  int local_pipe[2];
  int err = 0;

  int kidpipe[2];
  int kidchildpid;

#define READ	0
#define WRITE	1

  __tar_ck_pipe (local_pipe);

  childpid = fork ();
  if (childpid < 0)
    ERROR ((TAREXIT_FAILURE, errno, _("Cannot fork")));

  if (childpid > 0)
    {

      /* We're the parent.  Clean up and be happy.  This, at least, is
	 easy.  */

      if (ar_reading)
	{
	  flag_reblock++;
	  archive = local_pipe[READ];
	  __tar_ck_close (local_pipe[WRITE]);
	}
      else
	{
	  archive = local_pipe[WRITE];
	  __tar_ck_close (local_pipe[READ]);
	}
      return;
    }

  /* We're the kid.  */

  if (ar_reading)
    {
      __tar_dupto (local_pipe[WRITE], STDOUT, _("(child) Pipe to stdout"));
      __tar_ck_close (local_pipe[READ]);
    }
  else
    {
      __tar_dupto (local_pipe[READ], STDIN, _("(child) Pipe to stdin"));
      __tar_ck_close (local_pipe[WRITE]);
    }

  /* We need a child tar only if
     1: we're reading/writing stdin/out (to force reblocking),
     2: the file is to be accessed by rmt (compress doesn't know how),
     3: the file is not a plain file.  */

  if (strcmp (archive_name_array[0], "-") != 0
      && !_remdev (archive_name_array[0])
      && isfile (archive_name_array[0]))
    {

      /* We don't need a child tar.  Open the archive.  */

      if (ar_reading)
	{
	  archive = open (archive_name_array[0], O_RDONLY | O_BINARY, 0666);
	  if (archive < 0)
	    ERROR ((TAREXIT_FAILURE, errno, _("Cannot open archive %s"),
		    archive_name_array[0]));
	  __tar_dupto (archive, STDIN, _("Archive to stdin"));
#if 0
	  close (archive);
#endif
	}
      else
	{
	  archive = creat (archive_name_array[0], 0666);
	  if (archive < 0)
	    ERROR ((TAREXIT_FAILURE, errno, _("Cannot open archive %s"),
		    archive_name_array[0]));
	  __tar_dupto (archive, STDOUT, _("Archive to stdout"));
#if 0
	  close (archive);
#endif
	}
    }
  else
    {

      /* We need a child tar.  */

      __tar_ck_pipe (kidpipe);

      kidchildpid = fork ();
      if (kidchildpid < 0)
	ERROR ((TAREXIT_FAILURE, errno, _("Child cannot fork")));

      if (kidchildpid > 0)
	{

	  /* About to exec compress:  set up the files.  */

	  if (ar_reading)
	    {
	      __tar_dupto (kidpipe[READ], STDIN, _("((child)) Pipe to stdin"));
	      __tar_ck_close (kidpipe[WRITE]);
#if 0
	      dup2 (local_pipe[WRITE], STDOUT);
#endif
	    }
	  else
	    {
#if 0
	      dup2 (local_pipe[READ], STDIN);
#endif
	      __tar_dupto (kidpipe[WRITE], STDOUT, _("((child)) Pipe to stdout"));
	      __tar_ck_close (kidpipe[READ]);
	    }
#if 0
	  __tar_ck_close (local_pipe[READ]);
	  __tar_ck_close (local_pipe[WRITE]);
	  __tar_ck_close (kidpipe[READ]);
	  __tar_ck_close (kidpipe[WRITE]);
#endif
	}
      else
	{

	  /* Grandchild.  Do the right thing, namely sit here and
	     read/write the archive, and feed stuff back to compress.  */

	  program_name = _("tar (child)");
	  if (ar_reading)
	    {
	      __tar_dupto (kidpipe[WRITE], STDOUT, _("[child] Pipe to stdout"));
	      __tar_ck_close (kidpipe[READ]);
	    }
	  else
	    {
	      __tar_dupto (kidpipe[READ], STDIN, _("[child] Pipe to stdin"));
	      __tar_ck_close (kidpipe[WRITE]);
	    }

	  if (strcmp (archive_name_array[0], "-") == 0)
	    {
	      if (ar_reading)
		archive = STDIN;
	      else
		archive = STDOUT;
	    }
	  else
#if 0
	    /* This can't happen.  */

	    if (ar_reading==2)
	       archive
		 = rmtopen (archive_name_array[0], O_RDWR|O_CREAT | O_BINARY,
			    0666, flag_rsh_command);
	    else
#endif
	      if (ar_reading)
		archive = rmtopen (archive_name_array[0], O_RDONLY | O_BINARY,
				   0666, flag_rsh_command);
	      else
		archive = rmtcreat (archive_name_array[0], 0666, flag_rsh_command);

	  if (archive < 0)
	    ERROR ((TAREXIT_FAILURE, errno, _("Cannot open archive %s"),
		    archive_name_array[0]));

	  if (ar_reading)
	    {
	      while (1)
		{
		  char *ptr;
		  int max, count;

		  r_error_count = 0;

		error_loop:
		  err = rmtread (archive, ar_block->charptr,
				 (unsigned int) (blocksize));
		  if (err < 0)
		    {
		      __tar_readerror ();
		      goto error_loop;
		    }
		  if (err == 0)
		    break;
		  ptr = ar_block->charptr;
		  max = err;
		  while (max)
		    {
		      count = (max < RECORDSIZE) ? max : RECORDSIZE;
		      err = write (STDOUT, ptr, (size_t) count);
		      if (err < 0)
			ERROR ((TAREXIT_FAILURE, errno, _("\
Cannot write to compression program")));
		      
		      if (err != count)
			{
			  ERROR ((0, 0, _("\
Write to compression program short %d bytes"),
				     count - err));
			  count = err;
			}

		      ptr += count;
		      max -= count;
		    }
		}
	    }
	  else
	    {
	      while (1)
		{
		  int n;
		  char *ptr;

		  n = blocksize;
		  ptr = ar_block->charptr;
		  while (n)
		    {
		      err
			= read (STDIN, ptr,
				(size_t) ((n < RECORDSIZE) ? n : RECORDSIZE));
		      if (err <= 0)
			break;
		      n -= err;
		      ptr += err;
		    }

		  /* EOF */

		  if (err == 0)
		    {
		      if (!flag_compress_block)
			blocksize -= n;
		      else
			memset (ar_block->charptr + blocksize - n, 0, (size_t) n);
		      err = rmtwrite (archive, ar_block->charptr,
				      (unsigned int) blocksize);
		      if (err != (blocksize))
			__tar_writeerror (err);
		      if (!flag_compress_block)
			blocksize += n;
		      break;
		    }

		  if (n)
		    ERROR ((TAREXIT_FAILURE, errno,
			       _("Cannot read from compression program")));

		  err = rmtwrite (archive, ar_block->charptr,
				  (unsigned int) blocksize);
		  if (err != blocksize)
		    __tar_writeerror (err);
		}
	    }

#if 0
	  close_archive ();
#endif
	  exit (exit_status);
	}
    }

  /* So we should exec compress (-d).  */

  if (ar_reading)
    execlp (flag_compressprog, flag_compressprog, "-d", (char *) 0);
  else
    execlp (flag_compressprog, flag_compressprog, (char *) 0);

  /* In a previous tar version, _exit was used here instead of exit.  */
  ERROR ((TAREXIT_FAILURE, errno, _("Cannot exec %s"), flag_compressprog));
}

/*--------------------------------------------------.
| Return non-zero if P is the name of a directory.  |
`--------------------------------------------------*/

int
isfile (const char *p)
{
  struct stat stbuf;

  if (stat (p, &stbuf) < 0)
    return 1;
  if (S_ISREG (stbuf.st_mode))
    return 1;
  return 0;
}

#endif

/*------------------------------------------------------------------------.
| Open an archive file.  The argument specifies whether we are reading or |
| writing.								  |
`------------------------------------------------------------------------*/

/* JF if the arg is 2, open for reading and writing.  */

void
__tar_open_tar_archive (int reading)
{
  stdlis = flag_exstdout ? stderr : stdout;

  if (blocksize == 0)
    ERROR ((TAREXIT_FAILURE, 0, _("Invalid value for blocksize")));

  if (archive_names == 0)
    ERROR ((TAREXIT_FAILURE, 0,
	    _("No archive name given, what should I do?")));

  current_file_name = NULL;
  current_link_name = NULL;
  save_name = NULL;

  if (flag_multivol)
    {
      ar_block
	= (union record *) valloc ((unsigned) (blocksize + (2 * RECORDSIZE)));
      if (ar_block)
	ar_block += 2;
    }
  else
    ar_block = (union record *) valloc ((unsigned) blocksize);
  if (!ar_block)
    ERROR ((TAREXIT_FAILURE, 0,
	    _("Could not allocate memory for blocking factor %d"), blocking));

  ar_record = ar_block;
  ar_last = ar_block + blocking;
  ar_reading = reading;

  if (flag_multivol && flag_verify)
    ERROR ((TAREXIT_FAILURE, 0, _("Cannot verify multi-volume archives")));

  if (flag_compressprog)
    {
      if (reading == 2 || flag_verify)
	ERROR ((TAREXIT_FAILURE, 0,
		_("Cannot update or verify compressed archives")));
      if (flag_multivol)
	ERROR ((TAREXIT_FAILURE, 0,
		_("Cannot use multi-volume compressed archives")));
      __tar_child_open ();
      if (!reading && strcmp (archive_name_array[0], "-") == 0)
	stdlis = stderr;
#if 0
      __tar_child_open (rem_host, rem_file);
#endif
    }
  else if (strcmp (archive_name_array[0], "-") == 0)
    {
      flag_reblock++;		/* could be a pipe, be safe */
      if (flag_verify)
	ERROR ((TAREXIT_FAILURE, 0, _("Cannot verify stdin/stdout archive")));
      if (reading == 2)
	{
	  archive = STDIN;
	  stdlis = stderr;
	  write_archive_to_stdout++;
	}
      else if (reading)
	archive = STDIN;
      else
	{
	  archive = STDOUT;
	  stdlis = stderr;
	}
    }
  else if (reading == 2 || flag_verify)
    archive = rmtopen (archive_name_array[0], O_RDWR | O_CREAT | O_BINARY,
		       0666, flag_rsh_command);
  else if (reading)
    archive = rmtopen (archive_name_array[0], O_RDONLY | O_BINARY, 0666,
		       flag_rsh_command);
  else{
#ifdef __native_client__
      /*O_TRUNC, O_CREAT doesn't supported for zerovm channels, so unset truncate flag*/
      archive = open (archive_name_array[0], O_WRONLY, 0666);
#else
    archive = rmtcreat (archive_name_array[0], 0666, flag_rsh_command);
#endif
  }

  if (archive < 0)
    ERROR ((TAREXIT_FAILURE, errno, _("Cannot open %s"),
	    archive_name_array[0]));

#ifndef __MSDOS__
  if (!_isrmt (archive))
    {
      struct stat tmp_stat;

      fstat (archive, &tmp_stat);
      if (S_ISREG (tmp_stat.st_mode))
	{
	  ar_dev = tmp_stat.st_dev;
	  ar_ino = tmp_stat.st_ino;
	}
    }
#endif

#ifdef	__MSDOS__
  setmode (archive, O_BINARY);
#endif

  if (reading)
    {
      ar_last = ar_block;	/* set up for 1st block = # 0 */
      __tar_findrec ();		/* read it in, check for EOF */

      if (flag_volhdr)
	{
	  union record *label;
#if 0
	  char *ptr;

	  if (flag_multivol)
	    {
	      ptr = tar_xmalloc (strlen (flag_volhdr) + 20);
	      sprintf (ptr, "%s Volume %d", flag_volhdr, 1);
	    }
	  else
	    ptr = flag_volhdr;
#endif
	  label = __tar_findrec ();
	  if (!label)
	    ERROR ((TAREXIT_FAILURE, 0, _("Archive not labelled to match %s"),
		    flag_volhdr));
	  if (re_match (label_pattern, label->header.arch_name,
			(int) strlen (label->header.arch_name), 0, 0)
	      < 0)
	    ERROR ((TAREXIT_FAILURE, 0, _("Volume mismatch!  %s!=%s"),
		    flag_volhdr, label->header.arch_name));
#if 0
	  if (strcmp (ptr, label->header.name))
	      ERROR ((TAREXIT_FAILURE, 0, _("Volume mismatch!  %s!=%s"),
		      ptr, label->header.name));
	  if (ptr != flag_volhdr)
	    free (ptr);
#endif
	}
    }
  else if (flag_volhdr)
    {
      memset ((void *) ar_block, 0, RECORDSIZE);
      if (flag_multivol)
	sprintf (ar_block->header.arch_name, "%s Volume 1", flag_volhdr);
      else
	strcpy (ar_block->header.arch_name, flag_volhdr);

      __tar_assign_string (&current_file_name, ar_block->header.arch_name);

      ar_block->header.linkflag = LF_VOLHDR;
      __tar_to_oct (time (0), 1 + 12, ar_block->header.mtime);
      __tar_finish_header (ar_block);
#if 0
      ar_record++;
#endif
    }
}

/*------------------------------------------------------------------------.
| Remember a union record * as pointing to something that we need to keep |
| when reading onward in the file.  Only one such thing can be remembered |
| at once, and it only works when reading an archive.			  |
`------------------------------------------------------------------------*/

/* We calculate "offset" then add it because some compilers end up adding
   (baserec+ar_record), doing a 9-bit shift of baserec, then subtracting
   ar_block from that, shifting it back, losing the top 9 bits.  */

void
__tar_saverec (union record **pointer)
{
  long offset;

  save_rec = pointer;
  offset = ar_record - ar_block;
  saved_recno = baserec + offset;
}

/*--------------------------------------.
| Perform a write to flush the buffer.  |
`--------------------------------------*/

#if 0
   send_buffer_to_file ();
   if (__tar_new_volume)
     {
       deal_with___tar_new_volume_stuff ();
       send_buffer_to_file ();
     }
#endif

void
__tar_fl_write (void)
{
  int err;
  int copy_back;
  static long bytes_written = 0;

  if (flag_checkpoint && !(++checkpoint % 10))
    WARN ((0, 0, _("Write checkpoint %d"), checkpoint));

  if (tape_length && bytes_written >= tape_length * 1024)
    {
      errno = ENOSPC;
      err = 0;
    }
  else
    err = rmtwrite (archive, ar_block->charptr, (unsigned int) blocksize);
  if (err != blocksize && !flag_multivol)
    __tar_writeerror (err);
  else if (flag_totals)
    tot_written += blocksize;

  if (err > 0)
    bytes_written += err;
  if (err == blocksize)
    {
      if (flag_multivol)
	{
	  char *cursor;

	  if (!save_name)
	    {
	      real_s_name[0] = '\0';
	      real_s_totsize = 0;
	      real_s_sizeleft = 0;
	      return;
	    }

	  cursor = save_name;
#ifdef __MSDOS__
	  if (cursor[1] == ':')
	    cursor += 2;
#endif
	  while (*cursor == '/')
	    cursor++;

	  strcpy (real_s_name, cursor);
	  real_s_totsize = save_totsize;
	  real_s_sizeleft = save_sizeleft;
	}
      return;
    }

  /* We're multivol.  Panic if we didn't get the right kind of response.  */

  /* ENXIO is for the UNIX PC.  */
  if (err < 0 && errno != ENOSPC && errno != EIO && errno != ENXIO)
    __tar_writeerror (err);

  /* If error indicates a short write, we just move to the next tape.  */

  if (__tar_new_volume (0) < 0)
    return;
  bytes_written = 0;
  if (flag_volhdr && real_s_name[0])
    {
      copy_back = 2;
      ar_block -= 2;
    }
  else if (flag_volhdr || real_s_name[0])
    {
      copy_back = 1;
      ar_block--;
    }
  else
    copy_back = 0;
  if (flag_volhdr)
    {
      memset ((void *) ar_block, 0, RECORDSIZE);
      sprintf (ar_block->header.arch_name, "%s Volume %d", flag_volhdr, volno);
      __tar_to_oct (time (0), 1 + 12, ar_block->header.mtime);
      ar_block->header.linkflag = LF_VOLHDR;
      __tar_finish_header (ar_block);
    }
  if (real_s_name[0])
    {
      int tmp;

      if (flag_volhdr)
	ar_block++;
      memset ((void *) ar_block, 0, RECORDSIZE);
      strcpy (ar_block->header.arch_name, real_s_name);
      ar_block->header.linkflag = LF_MULTIVOL;
      __tar_to_oct ((long) real_s_sizeleft, 1 + 12,
	      ar_block->header.size);
      __tar_to_oct ((long) real_s_totsize - real_s_sizeleft,
	      1 + 12, ar_block->header.offset);
      tmp = flag_verbose;
      flag_verbose = 0;
      __tar_finish_header (ar_block);
      flag_verbose = tmp;
      if (flag_volhdr)
	ar_block--;
    }

  err = rmtwrite (archive, ar_block->charptr, (unsigned int) blocksize);
  if (err != blocksize)
    __tar_writeerror (err);
  else if (flag_totals)
    tot_written += blocksize;


  bytes_written = blocksize;
  if (copy_back)
    {
      ar_block += copy_back;
      memcpy ((void *) ar_record,
	      (void *) (ar_block + blocking - copy_back),
	      (size_t) (copy_back * RECORDSIZE));
      ar_record += copy_back;

      if (real_s_sizeleft >= copy_back * RECORDSIZE)
	real_s_sizeleft -= copy_back * RECORDSIZE;
      else if ((real_s_sizeleft + RECORDSIZE - 1) / RECORDSIZE <= copy_back)
	real_s_name[0] = '\0';
      else
	{
	  char *cursor = save_name;
	  
#ifdef __MSDOS__
	  if (cursor[1] == ':')
	    cursor += 2;
#endif
	  while (*cursor == '/')
	    cursor++;

	  strcpy (real_s_name, cursor);
	  real_s_sizeleft = save_sizeleft;
	  real_s_totsize = save_totsize;
	}
      copy_back = 0;
    }
}

/*---------------------------------------------------------------------.
| Handle write errors on the archive.  Write errors are always fatal.  |
| Hitting the end of a volume does not cause a write error unless the  |
| write was the first block of the volume.			       |
`---------------------------------------------------------------------*/

static void
__tar_writeerror (int err)
{
  if (err < 0)
    ERROR ((TAREXIT_FAILURE, errno, _("Cannot write to %s"),
	    *archive_name_cursor));
  else
    ERROR ((TAREXIT_FAILURE, 0, _("Only wrote %u of %u bytes to %s"),
	    err, blocksize, *archive_name_cursor));
}

/*-------------------------------------------------------------------.
| Handle read errors on the archive.  If the read should be retried, |
| __tar_readerror() returns to the caller.				     |
`-------------------------------------------------------------------*/

static void
__tar_readerror (void)
{
#define	READ_ERROR_MAX	10

  read_error_flag++;		/* tell callers */

  WARN ((0, errno, _("Read error on %s"), *archive_name_cursor));

  if (baserec == 0)
    ERROR ((TAREXIT_FAILURE, 0, _("At beginning of tape, quitting now")));

  /* Read error in mid archive.  We retry up to READ_ERROR_MAX times and
     then give up on reading the archive.  We set read_error_flag for our
     callers, so they can cope if they want.  */

  if (r_error_count++ > READ_ERROR_MAX)
    ERROR ((TAREXIT_FAILURE, 0, _("Too many errors, quitting")));
  return;
}

/*-------------------------------------.
| Perform a read to flush the buffer.  |
`-------------------------------------*/

void
__tar_fl_read (void)
{
  int err;			/* result from system call */
  int left;			/* bytes left */
  char *more;			/* pointer to next byte to read */

  if (flag_checkpoint && !(++checkpoint % 10))
    WARN ((0, 0, _("Read checkpoint %d"), checkpoint));

  /* Clear the count of errors.  This only applies to a single call to
     __tar_fl_read.  We leave read_error_flag alone; it is only turned off by
     higher level software.  */

  r_error_count = 0;		/* clear error count */

  /* If we are about to wipe out a record that somebody needs to keep,
     copy it out to a holding area and adjust somebody's pointer to it.  */

  if (save_rec &&
      *save_rec >= ar_record &&
      *save_rec < ar_last)
    {
      record_save_area = **save_rec;
      *save_rec = &record_save_area;
    }
  if (write_archive_to_stdout && baserec != 0)
    {
      err = rmtwrite (1, ar_block->charptr, (unsigned int) blocksize);
      if (err != blocksize)
	__tar_writeerror (err);
    }
  if (flag_multivol)
    if (save_name)
      {
	if (save_name != real_s_name)
	  {
#ifdef __MSDOS__
	    if (save_name[1] == ':')
	      save_name += 2;
#endif
	    while (*save_name == '/')
	      save_name++;

	    strcpy (real_s_name, save_name);
	    save_name = real_s_name;
	  }
	real_s_totsize = save_totsize;
	real_s_sizeleft = save_sizeleft;
      }
    else
      {
	real_s_name[0] = '\0';
	real_s_totsize = 0;
	real_s_sizeleft = 0;
      }

error_loop:
  err = rmtread (archive, ar_block->charptr, (unsigned int) blocksize);
  if (err == blocksize)
    return;

  if ((err == 0 || (err < 0 && errno == ENOSPC) || (err > 0 && !flag_reblock))
      && flag_multivol)
    {
      union record *cursor;

    try_volume:
      if (__tar_new_volume ((command_mode == COMMAND_APPEND
		       || command_mode == COMMAND_CAT
		       || command_mode == COMMAND_UPDATE) ? 2 : 1)
	  < 0)
	return;

    vol_error:
      err = rmtread (archive, ar_block->charptr, (unsigned int) blocksize);
      if (err < 0)
	{
	  __tar_readerror ();
	  goto vol_error;
	}
      if (err != blocksize)
	goto short_read;

      cursor = ar_block;

      if (cursor->header.linkflag == LF_VOLHDR)
	{
	  if (flag_volhdr)
	    {
#if 0
	      char *ptr;

	      ptr = (char *) tar_xmalloc (strlen (flag_volhdr) + 20);
	      sprintf (ptr, "%s Volume %d", flag_volhdr, volno);
#endif
	      if (re_match (label_pattern, cursor->header.arch_name,
			    (int) strlen (cursor->header.arch_name),
			    0, 0) < 0)
		{
		  WARN ((0, 0, _("Volume mismatch!  %s!=%s"),
			 flag_volhdr, cursor->header.arch_name));
		  volno--;
		  global_volno--;
		  goto try_volume;
		}

#if 0
	      if (strcmp (ptr, cursor->header.name))
		{
		  WARN ((0, 0, _("Volume mismatch!  %s!=%s"),
			 ptr, cursor->header.name));
		  volno--;
		  global_volno--;
		  free (ptr);
		  goto try_volume;
		}
	      free (ptr);
#endif
	    }
	  if (flag_verbose)
	    fprintf (stdlis, _("Reading %s\n"), cursor->header.arch_name);
	  cursor++;
	}
      else if (flag_volhdr)
	WARN ((0, 0, _("WARNING: No volume header")));

      if (real_s_name[0])
	{
	  if (cursor->header.linkflag != LF_MULTIVOL
	      || strcmp (cursor->header.arch_name, real_s_name))
	    {
	      WARN ((0, 0, _("%s is not continued on this volume"),
		     real_s_name));
	      volno--;
	      global_volno--;
	      goto try_volume;
	    }
	  if (real_s_totsize != (__tar_from_oct (1 + 12, cursor->header.size)
				 + __tar_from_oct (1 + 12, cursor->header.offset)))
	    {
	      WARN ((0, 0, _("%s is the wrong size (%ld != %ld + %ld)"),
			 cursor->header.arch_name, save_totsize,
			 __tar_from_oct (1 + 12, cursor->header.size),
			 __tar_from_oct (1 + 12, cursor->header.offset)));
	      volno--;
	      global_volno--;
	      goto try_volume;
	    }
	  if (real_s_totsize - real_s_sizeleft
	      != __tar_from_oct (1 + 12, cursor->header.offset))
	    {
	      WARN ((0, 0, _("This volume is out of sequence")));
	      volno--;
	      global_volno--;
	      goto try_volume;
	    }
	  cursor++;
	}
      ar_record = cursor;
      return;
    }
  else if (err < 0)
    {
      __tar_readerror ();
      goto error_loop;		/* try again */
    }

short_read:
  more = ar_block->charptr + err;
  left = blocksize - err;

again:
  if ((unsigned) left % RECORDSIZE == 0)
    {

      /* FIXME, for size=0, multi vol support.  On the first block, warn
	 about the problem.  */

      if (!flag_reblock && baserec == 0 && flag_verbose && err > 0)
	WARN ((0, 0, _("Blocksize = %d records"), err / RECORDSIZE));

      ar_last = ar_block + ((unsigned) (blocksize - left)) / RECORDSIZE;
      return;
    }
  if (flag_reblock)
    {

      /* User warned us about this.  Fix up.  */

      if (left > 0)
	{
	error2loop:
	  err = rmtread (archive, more, (unsigned int) left);
	  if (err < 0)
	    {
	      __tar_readerror ();
	      goto error2loop;	/* try again */
	    }
	  if (err == 0)
	    ERROR ((TAREXIT_FAILURE, 0,
		    _("Archive %s EOF not on block boundary"),
		    *archive_name_cursor));
	  left -= err;
	  more += err;
	  goto again;
	}
    }
  else
    ERROR ((TAREXIT_FAILURE, 0, _("Only read %d bytes from archive %s"),
	    err, *archive_name_cursor));
}

/*-----------------------------------------------.
| Flush the current buffer to/from the archive.	 |
`-----------------------------------------------*/

void
__tar_flush_archive (void)
{
  int c;

  baserec += ar_last - ar_block; /* keep track of block #s */
  ar_record = ar_block;		/* restore pointer to start */
  ar_last = ar_block + blocking; /* restore pointer to end */

  if (ar_reading)
    {
      if (time_to_start_writing)
	{
	  time_to_start_writing = 0;
	  ar_reading = 0;

	  if (file_to_switch_to >= 0)
	    {
	      if (c = rmtclose (archive), c < 0)
		WARN ((0, errno, _("WARNING: Cannot close %s (%d, %d)"),
		       *archive_name_cursor, archive, c));

	      archive = file_to_switch_to;
	    }
	  else
	    __tar_backspace_output ();
	  __tar_fl_write ();
	}
      else
	__tar_fl_read ();
    }
  else
    {
      __tar_fl_write ();
    }
}

/*-------------------------------------------------------------------------.
| Backspace the archive descriptor by one blocks worth.  If its a tape,	   |
| MTIOCTOP will work.  If its something else, we try to seek on it.  If we |
| can't seek, we lose!							   |
`-------------------------------------------------------------------------*/

static int
__tar_backspace_output (void)
{
  off_t cur;
#if 0
  int er;
#endif

#ifdef MTIOCTOP
  struct mtop t;

  t.mt_op = MTBSR;
  t.mt_count = 1;
  if (rmtioctl (archive, MTIOCTOP, (char *) &t) >= 0)
    return 1;
  if (errno == EIO && rmtioctl (archive, MTIOCTOP, (char *) &t) >= 0)
    return 1;
#endif

  cur = rmtlseek (archive, 0L, 1);
  cur -= blocksize;

  /* Seek back to the beginning of this block and start writing there.  */

  if (rmtlseek (archive, cur, 0) != cur)
    {

      /* Lseek failed.  Try a different method.  */

      WARN ((0, 0, _("\
Could not backspace archive file; it may be unreadable without -i")));

      /* Replace the first part of the block with nulls.  */

      if (ar_block->charptr != output_start)
	memset (ar_block->charptr, 0,
		(size_t) (output_start - ar_block->charptr));
      return 2;
    }
  return 3;
}

/*-------------------------.
| Close the archive file.  |
`-------------------------*/

void
close_tar_archive (void)
{
  int child;
  WAIT_T status;
  int c;

  if (time_to_start_writing || !ar_reading)
    __tar_flush_archive ();
  if (command_mode == COMMAND_DELETE)
    {
      off_t pos;

      pos = rmtlseek (archive, 0L, 1);
#ifndef __MSDOS__
      ftruncate (archive, (size_t) pos);
#else
      rmtwrite (archive, "", 0);
#endif
    }
  if (flag_verify)
    __tar_verify_volume ();

  if (c = rmtclose (archive), c < 0)
    WARN ((0, errno, _("WARNING: Cannot close %s (%d, %d)"),
	   *archive_name_cursor, archive, c));

#if !defined(__MSDOS__) && !defined(__native_client__)

  if (childpid)
    {

      /* Loop waiting for the right child to die, or for no more kids.  */

      while ((child = wait (&status), child != childpid) && child != -1)
	;

      if (child != -1)
	if (WIFSIGNALED (status)
#if 0
	    && !WIFSTOPPED (status)
#endif
	    )
	  {

	    /* SIGPIPE is OK, everything else is a problem.  */

	    if (WTERMSIG (status) != SIGPIPE)
	      ERROR ((0, 0, _("Child died with signal %d%s"),
		      WTERMSIG (status),
		      WCOREDUMP (status) ? _(" (core dumped)") : ""));
	  }
	else
	  {

	    /* Child voluntarily terminated -- but why?  /bin/sh returns
	       SIGPIPE + 120 if its child, then do nothing.  */

	    if (WEXITSTATUS (status) != (SIGPIPE + 128)
		&& WEXITSTATUS (status))
	      ERROR ((0, 0, _("Child returned status %d"),
		      WEXITSTATUS (status)));
	  }
    }
#endif /* not __MSDOS__ */

  if (current_file_name)
    free (current_file_name);
  if (current_link_name)
    free (current_link_name);
  if (save_name)
    free (save_name);
  free (flag_multivol ? ar_block - 2 : ar_block);
}

/*------------------------------------------------.
| Called to initialize the global volume number.  |
`------------------------------------------------*/

void
__tar_init_volume_number (void)
{
  FILE *vf;

  vf = fopen (flag_volno_file, "r");
  if (!vf && errno != ENOENT)
    ERROR ((0, errno, "%s", flag_volno_file));

  if (vf)
    {
      fscanf (vf, "%d", &global_volno);
      fclose (vf);
    }
}

/*-------------------------------------------------------.
| Called to write out the closing global volume number.	 |
`-------------------------------------------------------*/

void
__tar_closeout_volume_number (void)
{
  FILE *vf;

  vf = fopen (flag_volno_file, "w");
  if (!vf)
    ERROR ((0, errno, "%s", flag_volno_file));
  else
    {
      fprintf (vf, "%d\n", global_volno);
      fclose (vf);
    }
}

/*---------------------------------------------------------------------.
| We've hit the end of the old volume.  Close it and open the next one |
| Values for type: 0: writing 1: reading 2: updating		       |
`---------------------------------------------------------------------*/

static int
__tar_new_volume (int type)
{
  int c;
  char inbuf[80];
  const char *p;
  static FILE *read_file = 0;
  static int looped = 0;
  WAIT_T status;

  if (!read_file && !flag_run_script_at_end)
    read_file = (archive == 0) ? fopen (TTY_NAME, "r") : stdin;

  if (now_verifying)
    return -1;
  if (flag_verify)
    __tar_verify_volume ();
  if (c = rmtclose (archive), c < 0)
    WARN ((0, errno, _("WARNING: Cannot close %s (%d, %d)"),
	   *archive_name_cursor, archive, c));

  global_volno++;
  volno++;
  archive_name_cursor++;
  if (archive_name_cursor == archive_name_array + archive_names)
    {
      archive_name_cursor = archive_name_array;
      looped = 1;
    }

tryagain:
  if (looped)
    {

      /* We have to prompt from now on.  */

      if (flag_run_script_at_end)
	{
	  __tar_closeout_volume_number ();
	  system (info_script);
	}
      else
	while (1)
	  {
	    fprintf (stdlis,
		     _("\007Prepare volume #%d for %s and hit return: "),
		     global_volno, *archive_name_cursor);
	    fflush (stdlis);
	    if (fgets (inbuf, sizeof (inbuf), read_file) == 0)
	      {
		fprintf (stdlis, _("EOF?  What does that mean?"));

		if (command_mode != COMMAND_EXTRACT
		    && command_mode != COMMAND_LIST
		    && command_mode != COMMAND_DIFF)
		  WARN ((0, 0, _("WARNING: Archive is incomplete")));

		exit (TAREXIT_FAILURE);
	      }
	    if (inbuf[0] == '\n' || inbuf[0] == 'y' || inbuf[0] == 'Y')
	      break;

	    switch (inbuf[0])
	      {
	      case '?':
		{
		  fprintf (stdlis, _("\
 n [name]   Give a new filename for the next (and subsequent) volume(s)\n\
 q          Abort tar\n\
 !          Spawn a subshell\n\
 ?          Print this list\n"));
		}
		break;

	      case 'q':		/* quit */
		fprintf (stdlis, _("No new volume; exiting.\n"));

		if (command_mode != COMMAND_EXTRACT
		    && command_mode != COMMAND_LIST
		    && command_mode != COMMAND_DIFF)
		  WARN ((0, 0, _("WARNING: Archive is incomplete")));

		exit (TAREXIT_FAILURE);

	      case 'n':		/* get new file name */
		{
		  char *q, *r;

		  for (q = &inbuf[1]; *q == ' ' || *q == '\t'; q++)
		    ;
		  for (r = q; *r; r++)
		    if (*r == '\n')
		      *r = '\0';
		  r = (char *) tar_xmalloc ((size_t) (strlen (q) + 2));
		  strcpy (r, q);
		  p = r;
		  *archive_name_cursor = p;
		}
		break;

	      case '!':
#if 0 /*zerovm*/
#ifdef __MSDOS__
		spawnl (P_WAIT, getenv ("COMSPEC"), "-", 0);
#else
		switch (fork ())
		  {
		  case -1:
		    WARN ((0, errno, _("Cannot fork!")));
		    break;

		  case 0:
		    p = getenv ("SHELL");
		    if (p == 0)
		      p = "/bin/sh";
		    execlp (p, "-sh", "-i", 0);
		    /* In a previous tar version, _exit was used here
		       instead of exit.  */
		    ERROR ((TAREXIT_FAILURE, errno,
			    _("Cannot exec a shell %s"), p));

		  default:
		    /* zerovm
		       wait (&status); */
		    break;
		  }

		/* FIXME: I'm not sure if that's all that has to be done
		   here.  (jk)  */

#endif
#endif
		break;
	      }
	  }
    }

  if (type == 2 || flag_verify)
    archive = rmtopen (*archive_name_cursor, O_RDWR | O_CREAT, 0666,
		       flag_rsh_command);
  else if (type == 1)
    archive = rmtopen (*archive_name_cursor, O_RDONLY, 0666, flag_rsh_command);
  else if (type == 0)
    archive = rmtcreat (*archive_name_cursor, 0666, flag_rsh_command);
  else
    archive = -1;

  if (archive < 0)
    {
      WARN ((0, errno, _("Cannot open %s"), *archive_name_cursor));
      goto tryagain;
    }
#ifdef __MSDOS__
  setmode (archive, O_BINARY);
#endif
  return 0;
}

/*-------------------------------------------------------------------------.
| This is a useless function that takes a buffer returned by __tar_wantbytes and |
| does nothing with it.  If the function called by __tar_wantbytes returns an	   |
| error indicator (non-zero), this function is called for the rest of the  |
| file.									   |
`-------------------------------------------------------------------------*/

/* Yes, I know.  SIZE and DATA are unused in this function, and this
   might even trigger some compiler warnings.  That's OK!  Relax.  */

int
__tar_no_op (int size, char *data)
{
  return 0;
}

/*-----------------------------------------------------------------------.
| Some other routine wants SIZE bytes in the archive.  For each chunk of |
| the archive, call FUNC with the size of the chunk, and the address of	 |
| the chunk it can work with.						 |
`-----------------------------------------------------------------------*/

int
__tar_wantbytes (long size, int (*func) ())
{
  char *data;
  long data_size;

  if (flag_multivol)
    save_sizeleft = size;
  while (size)
    {
      data = __tar_findrec ()->charptr;
      if (data == NULL)
	{

	  /* Check it.  */

	  ERROR ((0, 0, _("Unexpected EOF on archive file")));
	  return -1;
	}
      data_size = __tar_endofrecs ()->charptr - data;
      if (data_size > size)
	data_size = size;
      if ((*func) (data_size, data))
	func = __tar_no_op;
      __tar_userec ((union record *) (data + data_size - 1));
      size -= data_size;
      if (flag_multivol)
	save_sizeleft -= data_size;
    }
  return 0;
}
