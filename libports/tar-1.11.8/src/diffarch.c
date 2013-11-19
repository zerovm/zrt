/* Diff files from a tar archive.
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

/* Diff files from a tar archive.
   Written 30 April 1987 by John Gilmore.  */

#include "system.h"

#ifndef S_ISLNK
#define lstat stat
#endif

#include "tar.h"
#include "rmt.h"

static int compare_chunk __P ((long, char *));
static int compare_dir __P ((long, char *));
static void diff_sparse_files __P ((int));
static int do_stat __P ((struct stat *));
static void fill_in_sparse_array __P ((void));

int now_verifying = 0;		/* are we verifying at the moment? */

/* Four tried statics!  */
static int diff_fd;		/* descriptor of file we're diffing */
static char *diff_buf = 0;	/* pointer to area for reading file contents
				   into */
static char *diff_dir;		/* directory contents for LF_DUMPDIR */

static int different = 0;

#if 0
struct sp_array *sparsearray;
int sp_ar_size = 10;
#endif

/*------------------------------------.
| Sigh about something that differs.  |
`------------------------------------*/

static void
sigh (const char *what)
{
  fprintf (stdlis, _("%s: %s differs\n"), current_file_name, what);
  if (exit_status == TAREXIT_SUCCESS)
    exit_status = TAREXIT_DIFFERS;
}

/*--------------------------------.
| Initialize for a diff operation |
`--------------------------------*/

void
diff_init (void)
{
  diff_buf = (char *) valloc ((unsigned) blocksize);
  if (!diff_buf)
    ERROR ((TAREXIT_FAILURE, 0,
	    _("Could not allocate memory for diff buffer of %d bytes"),
	    blocksize));
}

/*----------------------------------.
| Diff a file against the archive.  |
`----------------------------------*/

void
diff_archive (void)
{
  register char *data;
  int check, namelen;
  int err;
  off_t offset;
  struct stat filestat;

#ifndef __MSDOS__
  dev_t dev;
  ino_t ino;
#endif

  errno = EPIPE;		/* FIXME, remove perrors */

  saverec (&head);		/* make sure it sticks around */
  userec (head);		/* and go past it in the archive */
  decode_header (head, &hstat, &head_standard, 1);	/* snarf fields */

  /* Print the record from `head' and `hstat'.  */

  if (flag_verbose)
    {
      if (now_verifying)
	fprintf (stdlis, _("Verify "));
      print_header ();
    }

  switch (head->header.linkflag)
    {

    default:
      WARN ((0, 0, _("Unknown file type '%c' for %s, diffed as normal file"),
		 head->header.linkflag, current_file_name));
      /* Fall through.  */

    case LF_OLDNORMAL:
    case LF_NORMAL:
    case LF_SPARSE:
    case LF_CONTIG:

      /* Appears to be a file.  See if it's really a directory.  */

      namelen = strlen (current_file_name) - 1;
      if (current_file_name[namelen] == '/')
	goto really_dir;

      if (do_stat (&filestat))
	{
	  if (head->header.isextended)
	    skip_extended_headers ();
	  skip_file ((long) hstat.st_size);
	  different++;
	  goto quit;
	}

      if (!S_ISREG (filestat.st_mode))
	{
	  fprintf (stdlis, _("%s: Not a regular file\n"), current_file_name);
	  skip_file ((long) hstat.st_size);
	  different++;
	  goto quit;
	}

      filestat.st_mode &= 07777;
      if (filestat.st_mode != hstat.st_mode)
	sigh (_("Mode"));
      if (filestat.st_uid != hstat.st_uid)
	sigh (_("Uid"));
      if (filestat.st_gid != hstat.st_gid)
	sigh (_("Gid"));
      if (filestat.st_mtime != hstat.st_mtime)
	sigh (_("Mod time"));
      if (head->header.linkflag != LF_SPARSE &&
	  filestat.st_size != hstat.st_size)
	{
	  sigh (_("Size"));
	  skip_file ((long) hstat.st_size);
	  goto quit;
	}

      diff_fd = open (current_file_name, O_NDELAY | O_RDONLY | O_BINARY);

      if (diff_fd < 0 && !flag_absolute_names)
	{
	  char *tmpbuf = tar_xmalloc (strlen (current_file_name) + 2);

	  *tmpbuf = '/';
	  strcpy (tmpbuf + 1, current_file_name);
	  diff_fd = open (tmpbuf, O_NDELAY | O_RDONLY);
	  free (tmpbuf);
	}
      if (diff_fd < 0)
	{
	  ERROR ((0, errno, _("Cannot open %s"), current_file_name));
	  if (head->header.isextended)
	    skip_extended_headers ();
	  skip_file ((long) hstat.st_size);
	  different++;
	  goto quit;
	}

      /* Need to treat sparse files completely differently here.  */

      if (head->header.linkflag == LF_SPARSE)
	diff_sparse_files (hstat.st_size);
      else
	{
	  if (flag_multivol)
	    {
	      assign_string (&save_name, current_file_name);
	      save_totsize = hstat.st_size;
	      /* save_size is set in wantbytes ().  */
	    }
	  wantbytes ((long) (hstat.st_size), compare_chunk);
	  if (flag_multivol)
	    assign_string (&save_name, NULL);
	}

      check = close (diff_fd);
      if (check < 0)
	ERROR ((0, errno, _("Error while closing %s"), current_file_name));

    quit:
      break;

#ifndef __MSDOS__
    case LF_LINK:
      if (do_stat (&filestat))
	break;
      dev = filestat.st_dev;
      ino = filestat.st_ino;
      err = stat (current_link_name, &filestat);
      if (err < 0)
	{
	  if (errno == ENOENT)
	    fprintf (stdlis, _("%s: Does not exist\n"), current_file_name);
	  else
	    WARN ((0, errno, _("Cannot stat file %s"), current_file_name));
	  different++;
	  break;
	}
      if (filestat.st_dev != dev || filestat.st_ino != ino)
	{
	  fprintf (stdlis, _("%s: Not linked to %s\n"),
		   current_file_name, current_link_name);
	  break;
	}
      break;
#endif

#ifdef S_ISLNK
    case LF_SYMLINK:
      {
	char linkbuf[NAMSIZ + 3]; /* FIXME: may be too short.  */

	check = readlink (current_file_name, linkbuf, (sizeof linkbuf) - 1);

	if (check < 0)
	  {
	    if (errno == ENOENT)
	      fprintf (stdlis, _("%s: No such file or directory\n"),
		       current_file_name);
	    else
	      WARN ((0, errno, _("Cannot read link %s"), current_file_name));
	    different++;
	    break;
	  }

	linkbuf[check] = '\0';	/* null-terminate it */
	if (strncmp (current_link_name, linkbuf, (size_t) check) != 0)
	  {
	    fprintf (stdlis, _("%s: Symlink differs\n"), current_link_name);
	    different++;
	  }
      }
      break;
#endif

#ifdef S_IFCHR
    case LF_CHR:
      hstat.st_mode |= S_IFCHR;
      goto check_node;
#endif

#ifdef S_IFBLK
      /* If local system doesn't support block devices, use default case.  */

    case LF_BLK:
      hstat.st_mode |= S_IFBLK;
      goto check_node;
#endif

#ifdef S_ISFIFO
      /* If local system doesn't support FIFOs, use default case.  */

    case LF_FIFO:
#ifdef S_IFIFO
      hstat.st_mode |= S_IFIFO;
#endif
      hstat.st_rdev = 0;	/* FIXME, do we need this? */
      goto check_node;
#endif

    check_node:
      /* FIXME, deal with umask.  */

      if (do_stat (&filestat))
	break;
      if (hstat.st_rdev != filestat.st_rdev)
	{
	  fprintf (stdlis, _("%s: Device numbers changed\n"),
		   current_file_name);
	  different++;
	  break;
	}
      if (
#ifdef S_IFMT
	  hstat.st_mode != filestat.st_mode
#else
	  /* POSIX lossage */
	  (hstat.st_mode & 07777) != (filestat.st_mode & 07777)
#endif
	  )
	{
	  fprintf (stdlis, _("%s: Mode or device-type changed\n"),
		   current_file_name);
	  different++;
	  break;
	}
      break;

    case LF_DUMPDIR:
      data = diff_dir = get_dir_contents (current_file_name, 0);
      if (flag_multivol)
	{
	  assign_string (&save_name, current_file_name);
	  save_totsize = hstat.st_size;
	  /* save_size is set in wantbytes ().  */
	}
      if (data)
	{
	  wantbytes ((long) (hstat.st_size), compare_dir);
	  free (data);
	}
      else
	wantbytes ((long) (hstat.st_size), no_op);
      if (flag_multivol)
	assign_string (&save_name, NULL);
      /* Fall through.  */

    case LF_DIR:
      /* Check for trailing /.  */

      namelen = strlen (current_file_name) - 1;

    really_dir:
      while (namelen && current_file_name[namelen] == '/')
	current_file_name[namelen--] = '\0';	/* zap / */

      if (do_stat (&filestat))
	break;
      if (!S_ISDIR (filestat.st_mode))
	{
	  fprintf (stdlis, _("%s: No longer a directory\n"),
		   current_file_name);
	  different++;
	  break;
	}
      if ((filestat.st_mode & 07777) != (hstat.st_mode & 07777))
	sigh (_("Mode"));
      break;

    case LF_VOLHDR:
      break;

    case LF_MULTIVOL:
      namelen = strlen (current_file_name) - 1;
      if (current_file_name[namelen] == '/')
	goto really_dir;

      if (do_stat (&filestat))
	break;

      if (!S_ISREG (filestat.st_mode))
	{
	  fprintf (stdlis, _("%s: Not a regular file\n"), current_file_name);
	  skip_file ((long) hstat.st_size);
	  different++;
	  break;
	}

      filestat.st_mode &= 07777;
      offset = from_oct (1 + 12, head->header.offset);
      if (filestat.st_size != hstat.st_size + offset)
	{
	  sigh (_("Size"));
	  skip_file ((long) hstat.st_size);
	  different++;
	  break;
	}

      diff_fd = open (current_file_name, O_NDELAY | O_RDONLY | O_BINARY);

      if (diff_fd < 0)
	{
	  WARN ((0, errno, _("Cannot open file %s"), current_file_name));
	  skip_file ((long) hstat.st_size);
	  different++;
	  break;
	}
      err = lseek (diff_fd, offset, 0);
      if (err != offset)
	{
	  WARN ((0, errno, _("Cannot seek to %ld in file %s"),
		     offset, current_file_name));
	  different++;
	  break;
	}

      if (flag_multivol)
	{
	  assign_string (&save_name, current_file_name);
	  save_totsize = filestat.st_size;
	  /* save_size is set in wantbytes ().  */
	}
      wantbytes ((long) (hstat.st_size), compare_chunk);
      if (flag_multivol)
	assign_string (&save_name, NULL);

      check = close (diff_fd);
      if (check < 0)
	ERROR ((0, errno, _("Error while closing %s"), current_file_name));
      break;

    }

  /* We don't need to save it any longer. */

  saverec ((union record **) 0);	/* unsave it */
}

/*---.
| ?  |
`---*/

static int
compare_chunk (long bytes, char *buffer)
{
  int err;

  err = read (diff_fd, diff_buf, (size_t) bytes);
  if (err != bytes)
    {
      if (err < 0)
	WARN ((0, errno, _("Cannot read %s"), current_file_name));
      else
	fprintf (stdlis, _("%s: Could only read %d of %ld bytes\n"),
		 current_file_name, err, bytes);
      different++;
      return -1;
    }
  if (memcmp (buffer, diff_buf, (size_t) bytes))
    {
      fprintf (stdlis, _("%s: Data differs\n"), current_file_name);
      different++;
      return -1;
    }
  return 0;
}

/*---.
| ?  |
`---*/

static int
compare_dir (long bytes, char *buffer)
{
  if (memcmp (buffer, diff_dir, (size_t) bytes))
    {
      fprintf (stdlis, _("%s: Data differs\n"), current_file_name);
      different++;
      return -1;
    }
  diff_dir += bytes;
  return 0;
}

/*---.
| ?  |
`---*/

void
verify_volume (void)
{
  int status;
#ifdef MTIOCTOP
  struct mtop t;
  int er;
#endif

  if (!diff_buf)
    diff_init ();
#ifdef MTIOCTOP
  t.mt_op = MTBSF;
  t.mt_count = 1;
  if (er = rmtioctl (archive, MTIOCTOP, (char *) &t), er < 0)
    {
      if (errno != EIO
	  || (er = rmtioctl (archive, MTIOCTOP, (char *) &t), er < 0))
	{
#endif
	  if (rmtlseek (archive, 0L, 0) != 0)
	    {

	      /* Lseek failed.  Try a different method.  */

	      WARN ((0, errno, _("Could not rewind archive file for verify")));
	      return;
	    }
#ifdef MTIOCTOP
	}
    }
#endif
  ar_reading = 1;
  now_verifying = 1;
  fl_read ();
  while (1)
    {
      status = read_header ();
      if (status == 0)
	{
	  unsigned n;

	  n = 0;
	  do
	    {
	      n++;
	      status = read_header ();
	    }
	  while (status == 0);
	  ERROR ((0, 0,
		  _("VERIFY FAILURE: %d invalid header(s) detected"), n));
	}
      if (status == 2 || status == EOF)
	break;
      diff_archive ();
    }
  ar_reading = 0;
  now_verifying = 0;

}

/*---.
| ?  |
`---*/

static int
do_stat (struct stat *statp)
{
  int err;

  err = (flag_follow_links ? stat (current_file_name, statp)
	 : lstat (current_file_name, statp));
  if (err < 0)
    {
      if (errno == ENOENT)
	fprintf (stdlis, _("%s: Does not exist\n"), current_file_name);
      else
	ERROR ((0, errno, _("Cannot stat file %s"), current_file_name));
#if 0
      skip_file ((long) hstat.st_size);
      different++;
#endif
      return 1;
    }
  else
    return 0;
}

/*---.
| ?  |
`---*/

/* JK Diff'ing a sparse file with its counterpart on the tar file is a
   bit of a different story than a normal file.  First, we must know what
   areas of the file to skip through, i.e., we need to contruct a
   sparsearray, which will hold all the information we need.  We must
   compare small amounts of data at a time as we find it.  */

static void
diff_sparse_files (int filesize)
{
  int sparse_ind = 0;
  char *buf;
  int buf_size = RECORDSIZE;
  union record *datarec;
  int err;
  long numbytes;
#if 0
  int amt_read = 0;
#endif
  int size = filesize;

  /* FIXME: `datarec' might be used uninitialized in this function.
     Reported by Bruno Haible.  */

  buf = (char *) tar_xmalloc (buf_size * sizeof (char));

  fill_in_sparse_array ();


  while (size > 0)
    {
      datarec = findrec ();
      if (!sparsearray[sparse_ind].numbytes)
	break;

      /* `numbytes' is nicer to write than
	 `sparsearray[sparse_ind].numbytes' all the time...  */

      numbytes = sparsearray[sparse_ind].numbytes;

      lseek (diff_fd, sparsearray[sparse_ind].offset, 0);

      /* Take care to not run out of room in our buffer.  */

      while (buf_size < numbytes)
	{
	  buf_size *= 2;
	  buf = (char *) tar_realloc (buf, buf_size * sizeof (char));
	}
      while (numbytes > RECORDSIZE)
	{
	  if (err = read (diff_fd, buf, RECORDSIZE), err != RECORDSIZE)
	    {
	      if (err < 0)
		WARN ((0, errno, _("Cannot read %s"), current_file_name));
	      else
		fprintf (stdlis, _("%s: Could only read %d of %ld bytes\n"),
			 current_file_name, err, numbytes);
	      break;
	    }
	  if (memcmp (buf, datarec->charptr, RECORDSIZE))
	    {
	      different++;
	      break;
	    }
	  numbytes -= err;
	  size -= err;
	  userec (datarec);
	  datarec = findrec ();
	}
      if (err = read (diff_fd, buf, (size_t) numbytes), err != numbytes)
	{
	  if (err < 0)
	    WARN ((0, errno, _("Cannot read %s"), current_file_name));
	  else
	    fprintf (stdlis, _("%s: Could only read %d of %ld bytes\n"),
		     current_file_name, err, numbytes);
	  break;
	}

      if (memcmp (buf, datarec->charptr, (size_t) numbytes))
	{
	  different++;
	  break;
	}
#if 0
      amt_read += numbytes;
      if (amt_read >= RECORDSIZE)
	{
	  amt_read = 0;
	  userec (datarec);
	  datarec = findrec ();
	}
#endif
      userec (datarec);
      sparse_ind++;
      size -= numbytes;
    }

  /* If the number of bytes read isn't the number of bytes supposedly in
     the file, they're different.  */

#if 0
  if (amt_read != filesize)
    different++;
#endif

  userec (datarec);
  free (sparsearray);
  if (different)
    {
      fprintf (stdlis, _("%s: Data differs\n"), current_file_name);
      if (exit_status == TAREXIT_SUCCESS)
	exit_status = TAREXIT_DIFFERS;
    }
}

/*---.
| ?  |
`---*/

/* JK This routine should be used more often than it is ... look into
   that.  Anyhow, what it does is translate the sparse information on the
   header, and in any subsequent extended headers, into an array of
   structures with true numbers, as opposed to character strings.  It
   simply makes our life much easier, doing so many comparisong and such.
   */

static void
fill_in_sparse_array (void)
{
  int ind;

  /* Allocate space for our scratch space; it's initially 10 elements
     long, but can change in this routine if necessary.  */

  sp_array_size = 10;
  sparsearray = (struct sp_array *) tar_xmalloc (sp_array_size * sizeof (struct sp_array));

  /* There are at most five of these structures in the header itself;
     read these in first.  */

  for (ind = 0; ind < SPARSE_IN_HDR; ind++)
    {
      /* Compare to 0, or use !(int)..., for Pyramid's dumb compiler.  */
      if (head->header.sp[ind].numbytes == 0)
	break;
      sparsearray[ind].offset =
	from_oct (1 + 12, head->header.sp[ind].offset);
      sparsearray[ind].numbytes =
	from_oct (1 + 12, head->header.sp[ind].numbytes);
    }

  /* If the header's extended, we gotta read in exhdr's till we're done.  */

  if (head->header.isextended)
    {

      /* How far into the sparsearray we are `so far'.  */
      static int so_far_ind = SPARSE_IN_HDR;
      union record *exhdr;

      while (1)
	{
	  exhdr = findrec ();
	  for (ind = 0; ind < SPARSE_EXT_HDR; ind++)
	    {
	      if (ind + so_far_ind > sp_array_size - 1)
		{

		  /* We just ran out of room in our scratch area -
		     realloc it.  */

		  sp_array_size *= 2;
		  sparsearray = (struct sp_array *)
		    tar_realloc (sparsearray,
			      sp_array_size * sizeof (struct sp_array));
		}

	      /* Convert the character strings into longs.  */

	      sparsearray[ind + so_far_ind].offset =
		from_oct (1 + 12, exhdr->ext_hdr.sp[ind].offset);
	      sparsearray[ind + so_far_ind].numbytes =
		from_oct (1 + 12, exhdr->ext_hdr.sp[ind].numbytes);
	    }

	  /* If this is the last extended header for this file, we can
	     stop.  */

	  if (!exhdr->ext_hdr.isextended)
	    break;
	  else
	    {
	      so_far_ind += SPARSE_EXT_HDR;
	      userec (exhdr);
	    }
	}

      /* Be sure to skip past the last one.  */

      userec (exhdr);
    }
}
