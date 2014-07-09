/* Create a tar archive.
   Copyright (C) 1985, 1992, 1993, 1994 Free Software Foundation, Inc.

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

/* Create a tar archive.
   Written 25 Aug 1985 by John Gilmore.  */

#include "system.h"

#ifndef	__MSDOS__
#include <pwd.h>
#include <grp.h>
#endif

#ifdef HAVE_UTIME_H
#include <utime.h>
#else
struct utimbuf
  {
    long actime;
    long modtime;
  };
#endif

#include "tar.h"

extern struct stat hstat;	/* stat struct corresponding */

#ifndef __MSDOS__
extern dev_t ar_dev;
extern ino_t ar_ino;
#endif

/* JF */
extern struct name *gnu_list_name;

/* If there are no symbolic links, there is no lstat().  Use stat().  */
#ifndef S_ISLNK
#define lstat stat
#endif

/* This module is the only one that cares about `struct link's.  */

struct link
  {
    struct link *next;
    dev_t dev;
    ino_t ino;
    short linkcount;
    char name[1];
  };

struct link *linklist;		/* points to first link in list */


/*-------------------------------------------------------------------------.
| Quick and dirty octal conversion.  Converts long "value" into a	   |
| "digs"-digit field at "where", including a trailing space and room for a |
| null.  "digs"==3 means 1 digit, a space, and room for a null.		   |
| 									   |
| We assume the trailing null is already there and don't fill it in.  This |
| fact is used by __tar_start_header and __tar_finish_header, so don't change it!	   |
`-------------------------------------------------------------------------*/

/* This should be equivalent to:
	sprintf (where, "%*lo ", digs-2, value);
   except that sprintf fills in the trailing null and we don't.  */

void
__tar_to_oct (register long value, register int digs, register char *where)
{
  --digs;			/* Trailing null slot is left alone */
  where[--digs] = ' ';		/* put in the space, though */

  /* Produce the digits -- at least one.  */

  do
    {
      where[--digs] = '0' + (char) (value & 7);	/* one octal digit */
      value >>= 3;
    }
  while (digs > 0 && value != 0);

  /* Leading spaces, if necessary.  */
  while (digs > 0)
    where[--digs] = ' ';
}

/* Writing routines.  */

/*-----------------------------------------------------------------------.
| Just zeroes out the buffer so we don't confuse ourselves with leftover |
| data.									 |
`-----------------------------------------------------------------------*/

static void
__tar_clear_buffer (char *buf)
{
  register int i;

  for (i = 0; i < RECORDSIZE; i++)
    buf[i] = '\0';
}

/*------------------------------------------------------------------------.
| Write the EOT record(s).  We actually zero at least one record, through |
| the end of the block.  Old tar writes garbage after two zeroed records  |
| -- and PDtar used to.							  |
`------------------------------------------------------------------------*/

void
__tar_write_eot (void)
{
  union record *p;
  int bufsize;

  p = __tar_findrec ();
  if (p)
    {
      bufsize = __tar_endofrecs ()->charptr - p->charptr;
      memset (p->charptr, 0, (size_t) bufsize);
      __tar_userec (p);
    }
}

/*--------------------------------------------.
| Write a LF_LONGLINK or LF_LONGNAME record.  |
`--------------------------------------------*/

/* FIXME: Cross recursion between __tar_start_header and __tar_write_long!  */

static union record *__tar_start_header __P ((const char *, register struct stat *));
#if 0
static void __tar_write_long (const char *, char);
#endif

static void
__tar_write_long (const char *p, char type)
{
  int size = strlen (p) + 1;
  int bufsize;
  union record *header;
  struct stat foo;

  memset (&foo, 0, sizeof foo);
  foo.st_size = size;

  header = __tar_start_header ("././@LongLink", &foo);
  header->header.linkflag = type;
  __tar_finish_header (header);

  header = __tar_findrec ();

  bufsize = __tar_endofrecs ()->charptr - header->charptr;

  while (bufsize < size)
    {
      memcpy (header->charptr, p, (size_t) bufsize);
      p += bufsize;
      size -= bufsize;
      __tar_userec (header + (bufsize - 1) / RECORDSIZE);
      header = __tar_findrec ();
      bufsize = __tar_endofrecs ()->charptr - header->charptr;
    }
  memcpy (header->charptr, p, (size_t) size);
  memset (header->charptr + size, 0, (size_t) (bufsize - size));
  __tar_userec (header + (size - 1) / RECORDSIZE);
}

/* Header handling.  */

/*---------------------------------------------------------------------.
| Make a header block for the file name whose stat info is st.  Return |
| header pointer for success, NULL if the name is too long.	       |
`---------------------------------------------------------------------*/

static union record *
__tar_start_header (const char *name, register struct stat *st)
{
  register union record *header;

  if (strlen (name) >= (size_t) NAMSIZ)
    __tar_write_long (name, LF_LONGNAME);

  header = (union record *) __tar_findrec ();
  memset (header->charptr, 0, sizeof (*header));	/* FIXME: speed up */

  /* Check the file name and put it in the record.  */

  if (!flag_absolute_names)
    {
      static int warned_once = 0;

#ifdef __MSDOS__
      if (name[1] == ':')
	{
	  name += 2;
	  if (!warned_once++)
	    WARN ((0, 0, _("Removing drive spec from names in the archive")));
	}
#endif

      while (*name == '/')
	{
	  name++;		/* force relative path */
	  if (!warned_once++)
	    WARN ((0, 0, _("\
Removing leading / from absolute path names in the archive")));
	}
    }

  __tar_assign_string (&current_file_name, name);

  strncpy (header->header.arch_name, name, NAMSIZ);
  header->header.arch_name[NAMSIZ - 1] = '\0';

  /* Paul Eggert tried the trivial test ($WRITER cf a b; $READER tvf a)
     for a few tars and came up with the following interoperability
     matrix:

	      WRITER
	1 2 3 4 5 6 7 8 9   READER
	. . . . . . . . .   1 = SunOS 4.2 tar
	# . . # # . . # #   2 = NEC SVR4.0.2 tar
	. . . # # . . # .   3 = Solaris 2.1 tar
	. . . . . . . . .   4 = GNU tar 1.11.1
	. . . . . . . . .   5 = HP-UX 8.07 tar
	. . . . . . . . .   6 = Ultrix 4.1
	. . . . . . . . .   7 = AIX 3.2
	. . . . . . . . .   8 = Hitachi HI-UX 1.03
	. . . . . . . . .   9 = Omron UNIOS-B 4.3BSD 1.60Beta

	     . = works
	     # = ``impossible file type''

     The following mask for old archive removes the `#'s in column 4
     above, thus making GNU tar both a universal donor and a universal
     acceptor for Paul's test.  */

  __tar_to_oct ((long) (flag_oldarch ? (st->st_mode & 07777) : st->st_mode),
	  8, header->header.mode);

  __tar_to_oct ((long) st->st_uid, 8, header->header.uid);
  __tar_to_oct ((long) st->st_gid, 8, header->header.gid);
  __tar_to_oct ((long) st->st_size, 1 + 12, header->header.size);
  __tar_to_oct ((long) st->st_mtime, 1 + 12, header->header.mtime);

  /* header->header.linkflag is left as null.  */

  if (flag_gnudump)
    {
      __tar_to_oct ((long) st->st_atime, 1 + 12, header->header.atime);
      __tar_to_oct ((long) st->st_ctime, 1 + 12, header->header.ctime);
    }

#ifndef NONAMES
  /* Fill in new Unix Standard fields if desired.  */

  if (flag_standard)
    {
      header->header.linkflag = LF_NORMAL;	/* new default */
      strcpy (header->header.magic, TMAGIC);	/* mark as Unix Std */
      __tar_finduname (header->header.uname, st->st_uid);
      __tar_findgname (header->header.gname, st->st_gid);
    }
#endif

  return header;
}

/*-------------------------------------------------------------------------.
| Finish off a filled-in header block and write it out.  We also print the |
| file name and/or full info if verbose is on.				   |
`-------------------------------------------------------------------------*/

void
__tar_finish_header (register union record *header)
{
  register int i, sum;
  register char *p;

  /*avoid adding empty name extracted from root name, in case if archiving root folder*/
  if ( !header->header.arch_name || !strlen(header->header.arch_name) ) return;

  memcpy (header->header.chksum, CHKBLANKS, sizeof (header->header.chksum));

  sum = 0;
  p = header->charptr;
  for (i = sizeof (*header); --i >= 0; )
    /* We can't use unsigned char here because of old compilers, e.g. V7.  */
    sum += 0xFF & *p++;

  /* Fill in the checksum field.  It's formatted differently from the
     other fields: it has [6] digits, a null, then a space -- rather than
     digits, a space, then a null.  We use __tar_to_oct then write the null in
     over __tar_to_oct's space.  The final space is already there, from
     checksumming, and __tar_to_oct doesn't modify it.

     This is a fast way to do:

     sprintf(header->header.chksum, "%6o", sum);  */

  __tar_to_oct ((long) sum, 8, header->header.chksum);
  header->header.chksum[6] = '\0';	/* zap the space */

  __tar_userec (header);

  if (flag_verbose)
    {

      /* These globals are parameters to __tar_print_header, sigh.  */

      head = header;
      /* hstat is already set up.  */
      head_standard = flag_standard;
      __tar_print_header ();
    }

  return;
}

/* Sparse file processing.  */

/*---------------------------------------------------------------------.
| Takes a recordful of data and basically cruises through it to see if |
| it's made *entirely* of zeros, returning a 0 the instant it finds    |
| something that is a non-zero, i.e., useful data.		       |
`---------------------------------------------------------------------*/

static int
__tar_zero_record (char *buffer)
{
  register int i;

  for (i = 0; i < RECORDSIZE; i++)
    if (buffer[i] != '\0')
      return 0;
  return 1;
}

/*---.
| ?  |
`---*/

static void
__tar_init_sparsearray (void)
{
  register int i;

  sp_array_size = 10;

  /* Make room for our scratch space -- initially is 10 elts long.  */

  sparsearray = (struct sp_array *)
    tar_xmalloc (sp_array_size * sizeof (struct sp_array));
  for (i = 0; i < sp_array_size; i++)
    {
      sparsearray[i].offset = 0;
      sparsearray[i].numbytes = 0;
    }
}

/*---.
| ?  |
`---*/

static void
__tar_find_new_file_size (int *filesize, int highest_index)
{
  register int i;

  *filesize = 0;
  for (i = 0; sparsearray[i].numbytes && i <= highest_index; i++)
    *filesize += sparsearray[i].numbytes;
}

/*-------------------------------------------------------------------------.
| Okay, we've got a sparse file on our hands -- now, what we need to do is |
| make a pass through the file and carefully note where any data is, i.e., |
| we want to find how far into the file each instance of data is, and how  |
| many bytes are there.  We store this information in the sparsearray,	   |
| which will later be translated into header information.  For now, we use |
| the sparsearray as convenient storage.				   |
`-------------------------------------------------------------------------*/

/* As a side note, this routine is a mess.  If I could have found a
   cleaner way to do it, I would have.  If anyone wants to find a nicer
   way to do this, feel free.

   There is little point in trimming small amounts of null data at the
   head and tail of blocks -- it's ok if we only avoid dumping blocks of
   complete null data.  */

static int
__tar_deal_with_sparse (char *name, union record *header)
{
  long numbytes = 0;
  long offset = 0;
#if 0
  long save_offset;
#endif
  int fd;
#if 0
  int current_size = hstat.st_size;
#endif
  int sparse_ind = 0, cc;
  char buf[RECORDSIZE];
#if 0
  int read_last_data = 0;	/* did we just read the last record? */
#endif
  int amidst_data = 0;

  header->header.isextended = 0;

  /* Cannot open the file -- this problem will be caught later on, so just
     return.  */

  if (fd = open (name, O_RDONLY), fd < 0)
    return 0;

  __tar_init_sparsearray ();
  __tar_clear_buffer (buf);

  while (cc = read (fd, buf, sizeof buf), cc != 0)
    {
      if (sparse_ind > sp_array_size - 1)
	{

	  /* Realloc the scratch area, since we've run out of room.  */

	  sparsearray = (struct sp_array *)
	    tar_realloc (sparsearray,
		      2 * sp_array_size * sizeof (struct sp_array));
	  sp_array_size *= 2;
	}
      if (cc == sizeof buf)
	{
	  if (__tar_zero_record (buf))
	    {
	      if (amidst_data)
		{
		  sparsearray[sparse_ind++].numbytes = numbytes;
		  amidst_data = 0;
		}
	    }
	  else
	    if (amidst_data)
	      numbytes += cc;
	    else
	      {
		amidst_data = 1;
		numbytes = cc;
		sparsearray[sparse_ind].offset = offset;
	      }
	}
      else if (cc < sizeof buf)
	{

	  /* This has to be the last bit of the file, so this is somewhat
	     shorter than the above.  */

	  if (!__tar_zero_record (buf))
	    {
	      if (amidst_data)
		numbytes += cc;
	      else
		{
		  amidst_data = 1;
		  numbytes = cc;
		  sparsearray[sparse_ind].offset = offset;
		}
	    }
	}
      offset += cc;
      __tar_clear_buffer (buf);
    }
  if (amidst_data)
    sparsearray[sparse_ind++].numbytes = numbytes;
  else
    {
      sparsearray[sparse_ind].offset = offset - 1;
      sparsearray[sparse_ind++].numbytes = 1;
    }
  close (fd);

  return sparse_ind - 1;
}

/*---.
| ?  |
`---*/

static int
__tar_finish_sparse_file (int fd, long *sizeleft, long fullsize, char *name)
{
  union record *start;
  char tempbuf[RECORDSIZE];
  int bufsize, sparse_ind = 0, count;
  long pos;
  long nwritten = 0;

  while (*sizeleft > 0)
    {
      start = __tar_findrec ();
      memset (start->charptr, 0, RECORDSIZE);
      bufsize = sparsearray[sparse_ind].numbytes;
      if (!bufsize)
	{

	  /* We blew it, maybe.  */

	  ERROR ((0, 0, _("Wrote %ld of %ld bytes to file %s"),
		  fullsize - *sizeleft, fullsize, name));
	  break;
	}
      pos = lseek (fd, sparsearray[sparse_ind++].offset, 0);

      /* If the number of bytes to be written here exceeds the size of
	 the temporary buffer, do it in steps.  */

      while (bufsize > RECORDSIZE)
	{
#if 0
	  if (amt_read)
	    {
	      count = read (fd, start->charptr+amt_read, RECORDSIZE-amt_read);
	      bufsize -= RECORDSIZE - amt_read;
	      amt_read = 0;
	      __tar_userec (start);
	      start = __tar_findrec ();
	      memset (start->charptr, 0, RECORDSIZE);
	    }
#endif
	  /* Store the data.  */

	  count = read (fd, start->charptr, RECORDSIZE);
	  if (count < 0)
	    {
	      ERROR ((0, errno, _("\
Read error at byte %ld, reading %d bytes, in file %s"),
			 fullsize - *sizeleft, bufsize, name));
	      return 1;
	    }
	  bufsize -= count;
	  *sizeleft -= count;
	  __tar_userec (start);
	  nwritten += RECORDSIZE;	/* FIXME */
	  start = __tar_findrec ();
	  memset (start->charptr, 0, RECORDSIZE);
	}

      __tar_clear_buffer (tempbuf);
      count = read (fd, tempbuf, (size_t) bufsize);
      memcpy (start->charptr, tempbuf, RECORDSIZE);
      if (count < 0)
	{
	  ERROR ((0, errno,
		  _("Read error at byte %ld, reading %d bytes, in file %s"),
		  fullsize - *sizeleft, bufsize, name));
	  return 1;
	}
#if 0
      if (amt_read >= RECORDSIZE)
	{
	  amt_read = 0;
	  __tar_userec (start + (count - 1) / RECORDSIZE);
	  if (count != bufsize)
	    {
	      ERROR ((0, 0,
		      _("File %s shrunk by %d bytes, padding with zeros"),
		      name, sizeleft));
	      return 1;
	    }
	  start = __tar_findrec ();
	}
      else
	amt_read += bufsize;
#endif
      nwritten += count;	/* FIXME */
      *sizeleft -= count;
      __tar_userec (start);

    }
  free (sparsearray);
#if 0
  printf (_("Amount actually written is (I hope) %d.\n"), nwritten);
  __tar_userec (start + (count - 1) / RECORDSIZE);
#endif
  return 0;
}

/* Main functions of this module.  */

/*---.
| ?  |
`---*/

void
__tar_create_archive (void)
{
  register char *p;

  __tar_open_tar_archive (0);		/* open for writing */

  if (flag_gnudump)
    {
      char *buf = tar_xmalloc (PATH_MAX);
      char *q, *bufp;

      __tar_collect_and_sort_names ();

      while (p = __tar_name_from_list (), p)
	__tar_dump_file (p, -1, 1);
#if 0
      if (!flag_dironly)
	{
#endif
	  __tar_blank_name_list ();
	  while (p = __tar_name_from_list (), p)
	    {
	      strcpy (buf, p);
	      if (p[strlen (p) - 1] != '/')
		strcat (buf, "/");
	      bufp = buf + strlen (buf);
	      for (q = gnu_list_name->dir_contents;
		   q && *q;
		   q += strlen (q) + 1)
		{
		  if (*q == 'Y')
		    {
		      strcpy (bufp, q + 1);
		      __tar_dump_file (buf, -1, 1);
		    }
		}
	    }
#if 0
	}
#endif
      free (buf);
    }
  else
    {
      while (p = __tar_name_next (1), p)
	__tar_dump_file (p, -1, 1);
    }

  __tar_write_eot ();
  close_tar_archive ();
  if (flag_gnudump && gnu_dumpfile)
    __tar_write_dir_file ();
  __tar_name_close ();
}

/*-------------------------------------------------------------------------.
| Dump a single file.  If it's a directory, recurse.  Result is 1 for	   |
| success, 0 for failure.  Sets global "hstat" to stat() output for this   |
| file.  P is file name to dump.  CURDEV is device our parent dir was on.  |
| TOPLEVEL tells wether we are a toplevel call.				   |
`-------------------------------------------------------------------------*/

void
__tar_dump_file (char *p, int curdev, int toplevel)
{
  union record *header;
  char type;
  union record *exhdr;
  char save_linkflag;
  int critical_error = 0;
  struct utimbuf restore_times;
#if 0
  int sparse_ind = 0;
#endif

  /* FIXME: `header' might be used uninitialized in this function.
     Reported by Bruno Haible.  */

  /* FIXME: `upperbound' might be used uninitialized in this function.
     Reported by Bruno Haible.  */

  if (flag_confirm && !__tar_confirm ("add", p))
    return;

  /* Use stat if following (rather than dumping) 4.2BSD's symbolic links.
     Otherwise, use lstat (which, on non-4.2 systems, is #define'd to
     stat anyway.  */

  DEBUG_PRINT("c1");
  DEBUG_PRINT(p);
  if( stat (p, &hstat) ){
      DEBUG_PRINT("err13");
  }
  if( lstat (p, &hstat) ){
      DEBUG_PRINT("err14");
  }

  DEBUG_PRINT("c3");

#ifdef STX_HIDDEN		/* AIX */
  if (flag_follow_links != 0 ?
      statx (p, &hstat, STATSIZE, STX_HIDDEN) :
      statx (p, &hstat, STATSIZE, STX_HIDDEN | STX_LINK))
#else
  if (flag_follow_links != 0 ? stat (p, &hstat) : lstat (p, &hstat))
#endif
    {
    badperror:
      WARN ((0, errno, _("Cannot add file %s"), p));
      /* Fall through.  */

    badfile:
      if (!flag_ignore_failed_read || critical_error)
	exit_status = TAREXIT_FAILURE;

      return;
    }

  restore_times.actime = hstat.st_atime;
  restore_times.modtime = hstat.st_mtime;

#ifdef S_ISHIDDEN
  if (S_ISHIDDEN (hstat.st_mode))
    {
      char *new = (char *) alloca (strlen (p) + 2);
      if (new)
	{
	  strcpy (new, p);
	  strcat (new, "@");
	  p = new;
	}
    }
#endif

  /* See if we only want new files, and check if this one is too old to
     put in the archive.  */

  if (flag_new_files && !flag_gnudump && new_time > hstat.st_mtime
      && !S_ISDIR (hstat.st_mode)
      && (flag_new_files > 1 || new_time > hstat.st_ctime))
    {
      if (curdev == -1)
	WARN ((0, 0, _("%s: is unchanged; not dumped"), p));
      return;
    }

#ifndef __MSDOS__
  /* See if we are trying to dump the archive.  */

  if (ar_dev && hstat.st_dev == ar_dev && hstat.st_ino == ar_ino)
    {
      WARN ((0, 0, _("%s is the archive; not dumped"), p));
      return;
    }
#endif

  /* Check for multiple links.

     We maintain a list of all such files that we've written so far.  Any
     time we see another, we check the list and avoid dumping the data
     again if we've done it once already.  */

  if (hstat.st_nlink > 1
      && (S_ISREG (hstat.st_mode)
#ifdef S_ISCTG
	  || S_ISCTG (hstat.st_mode)
#endif
#ifdef S_ISCHR
	  || S_ISCHR (hstat.st_mode)
#endif
#ifdef S_ISBLK
	  || S_ISBLK (hstat.st_mode)
#endif
#ifdef S_ISFIFO
	  || S_ISFIFO (hstat.st_mode)
#endif
      ))
    {
      register struct link *lp;

      /* First quick and dirty.  Hashing, etc later.  FIXME.  */

      for (lp = linklist; lp; lp = lp->next)
	if (lp->ino == hstat.st_ino && lp->dev == hstat.st_dev)
	  {
	    char *link_name = lp->name;

	    /* We found a link.  */

	    while (!flag_absolute_names && *link_name == '/')
	      {
		static int link_warn = 0;

		if (!link_warn)
		  {
		    WARN ((0, 0, _("Removing leading / from absolute links")));
		    link_warn++;
		  }
		link_name++;
	      }
	    if (link_name - lp->name >= NAMSIZ)
	      __tar_write_long (link_name, LF_LONGLINK);
	    __tar_assign_string (&current_link_name, link_name);

	    hstat.st_size = 0;
	    header = __tar_start_header (p, &hstat);
	    if (header == NULL)
	      {
		critical_error = 1;
		goto badfile;
	      }
	    strncpy (header->header.arch_linkname,
		     link_name, NAMSIZ);

	    /* Force null truncated.  */

	    header->header.arch_linkname[NAMSIZ - 1] = 0;

	    header->header.linkflag = LF_LINK;
	    __tar_finish_header (header);

	    /* FIXME: Maybe remove from list after all links found?  */

	    if (flag_remove_files)
	      {
		if (unlink (p) == -1)
		  ERROR ((0, errno, _("Cannot remove %s"), p));
	      }
	    return;		/* we dumped it */
	  }

      /* Not found.  Add it to the list of possible links.  */

      lp = (struct link *)
	tar_xmalloc ((size_t) (sizeof (struct link) + strlen (p)));
      lp->ino = hstat.st_ino;
      lp->dev = hstat.st_dev;
      strcpy (lp->name, p);
      lp->next = linklist;
      linklist = lp;
    }

  /* This is not a link to a previously dumped file, so dump it.  */

  if (S_ISREG (hstat.st_mode)
#ifdef S_ISCTG
      || S_ISCTG (hstat.st_mode)
#endif
      )
    {
      int f;			/* file descriptor */
      long bufsize, count;
      long sizeleft;
      register union record *start;
      int header_moved;
      char isextended = 0;
      int upperbound;
#if 0
      int end_nulls = 0;
#endif
#if 0
      static int cried_once = 0;
#endif

      header_moved = 0;

      if (flag_sparse_files)
	{

	  /* Check the size of the file against the number of blocks
	     allocated for it, counting both data and indirect blocks.
	     If there is a smaller number of blocks that would be
	     necessary to accommodate a file of this size, this is safe
	     to say that we have a sparse file: at least one of those
	     records in the file is just a useless hole.  For sparse
	     files not having more hole blocks than indirect blocks, the
	     sparseness will go undetected.  */

	  /* tar.h defines ST_NBLOCKS in term of 512 byte sectors, even
	     for HP-UX's which count in 1024 byte units and AIX's which
	     count in 4096 byte units.  So this should work...  */

	  /* Bruno Haible sent me these statistics for Linux.  It seems that
	     some filesystems count indirect blocks in st_blocks, while
	     others do not seem to:

	     minix-fs   tar: size=7205, st_blocks=18 and ST_NBLOCKS=18
	     extfs      tar: size=7205, st_blocks=18 and ST_NBLOCKS=18
	     ext2fs     tar: size=7205, st_blocks=16 and ST_NBLOCKS=16
	     msdos-fs   tar: size=7205, st_blocks=16 and ST_NBLOCKS=16 */

	  if (hstat.st_size > ST_NBLOCKS (hstat) * RECORDSIZE)
	    {
	      int filesize = hstat.st_size;
	      register int i;

	      header = __tar_start_header (p, &hstat);
	      if (header == NULL)
		{
		  critical_error = 1;
		  goto badfile;
		}
	      header->header.linkflag = LF_SPARSE;
	      header_moved++;

	      /* Call the routine that figures out the layout of the
		 sparse file in question.  UPPERBOUND is the index of the
		 last element of the "sparsearray," i.e., the number of
		 elements it needed to describe the file.  */

	      upperbound = __tar_deal_with_sparse (p, header);

	      /* See if we'll need an extended header later.  */

	      if (upperbound > SPARSE_IN_HDR - 1)
		header->header.isextended++;

	      /* We store the "real" file size so we can show that in
		 case someone wants to list the archive, i.e., tar tvf
		 <file>.  It might be kind of disconcerting if the
		 shrunken file size was the one that showed up.  */

	      __tar_to_oct ((long) hstat.st_size, 1 + 12, header->header.realsize);

	      /* This will be the new "size" of the file, i.e., the size
		 of the file minus the records of holes that we're
		 skipping over.  */

	      __tar_find_new_file_size (&filesize, upperbound);
	      hstat.st_size = filesize;
	      __tar_to_oct ((long) filesize, 1 + 12, header->header.size);
#if 0
	      __tar_to_oct ((long) end_nulls, 1 + 12, header->header.ending_blanks);
#endif

	      for (i = 0; i < SPARSE_IN_HDR; i++)
		{
		  if (!sparsearray[i].numbytes)
		    break;
		  __tar_to_oct (sparsearray[i].offset, 1 + 12,
			  header->header.sp[i].offset);
		  __tar_to_oct (sparsearray[i].numbytes, 1 + 12,
			  header->header.sp[i].numbytes);
		}

	    }
	}
      else
	upperbound = SPARSE_IN_HDR - 1;

      sizeleft = hstat.st_size;

      /* Don't bother opening empty, world readable files.  */

      if (sizeleft > 0 || 0444 != (0444 & hstat.st_mode))
	{
	  f = open (p, O_RDONLY | O_BINARY);
	  if (f < 0)
	    goto badperror;
	}
      else
	f = -1;

      /* If the file is sparse, we've already taken care of this.  */

      if (!header_moved)
	{
	  header = __tar_start_header (p, &hstat);
	  if (header == NULL)
	    {
	      if (f >= 0)
		close (f);
	      critical_error = 1;
	      goto badfile;
	    }
	}
#ifdef S_ISCTG
      /* Mark contiguous files, if we support them.  */

      if (flag_standard && S_ISCTG (hstat.st_mode))
	header->header.linkflag = LF_CONTIG;
#endif
      isextended = header->header.isextended;
      save_linkflag = header->header.linkflag;
      __tar_finish_header (header);
      if (isextended)
	{
#if 0
	  int sum = 0;
#endif
	  register int i;
#if 0
	  register union record *exhdr;
	  int arraybound = SPARSE_EXT_HDR;
#endif
	  /* static */ int index_offset = SPARSE_IN_HDR;

	extend:
	  exhdr = __tar_findrec ();

	  if (exhdr == NULL)
	    {
	      critical_error = 1;
	      goto badfile;
	    }
	  memset (exhdr->charptr, 0, RECORDSIZE);
	  for (i = 0; i < SPARSE_EXT_HDR; i++)
	    {
	      if (i + index_offset > upperbound)
		break;
	      __tar_to_oct ((long) sparsearray[i + index_offset].numbytes,
		      1 + 12,
		      exhdr->ext_hdr.sp[i].numbytes);
	      __tar_to_oct ((long) sparsearray[i + index_offset].offset,
		      1 + 12,
		      exhdr->ext_hdr.sp[i].offset);
	    }
	  __tar_userec (exhdr);
#if 0
	  sum += i;
	  if (sum < upperbound)
	    goto extend;
#endif
	  if (index_offset + i <= upperbound)
	    {
	      index_offset += i;
	      exhdr->ext_hdr.isextended++;
	      goto extend;
	    }

	}
      if (save_linkflag == LF_SPARSE)
	{
	  if (__tar_finish_sparse_file (f, &sizeleft, hstat.st_size, p))
	    goto padit;
	}
      else
	while (sizeleft > 0)
	  {
	    if (flag_multivol)
	      {
		__tar_assign_string (&save_name, p);
		save_sizeleft = sizeleft;
		save_totsize = hstat.st_size;
	      }
	    start = __tar_findrec ();

	    bufsize = __tar_endofrecs ()->charptr - start->charptr;

	    if (sizeleft < bufsize)
	      {

		/* Last read -- zero out area beyond.  */

		bufsize = (int) sizeleft;
		count = bufsize % RECORDSIZE;
		if (count)
		  memset (start->charptr + sizeleft, 0,
			  (size_t) (RECORDSIZE - count));
	      }
	    count = read (f, start->charptr, (size_t) bufsize);
	    if (count < 0)
	      {
		ERROR ((0, errno, _("\
Read error at byte %ld, reading %d bytes, in file %s"),
			   hstat.st_size - sizeleft, bufsize, p));
		goto padit;
	      }
	    sizeleft -= count;

	    /* This is nonportable (the type of userec's arg).  */

	    __tar_userec (start + (count - 1) / RECORDSIZE);

	    if (count == bufsize)
	      continue;
	    ERROR ((0, 0, _("File %s shrunk by %d bytes, padding with zeros"),
		    p, sizeleft));
	    goto padit;		/* short read */
	  }

      if (flag_multivol)
	__tar_assign_string (&save_name, NULL);

      if (f >= 0)
	close (f);

      if (flag_remove_files)
	{
	  if (unlink (p) == -1)
	    ERROR ((0, errno, _("Cannot remove %s"), p));
	}
      if (flag_atime_preserve)
	utime (p, &restore_times);
      return;

      /* File shrunk or gave error, pad out tape to match the size we
	 specified in the header.  */

    padit:
      while (sizeleft > 0)
	{
	  save_sizeleft = sizeleft;
	  start = __tar_findrec ();
	  memset (start->charptr, 0, RECORDSIZE);
	  __tar_userec (start);
	  sizeleft -= RECORDSIZE;
	}
      if (flag_multivol)
	__tar_assign_string (&save_name, NULL);
      if (f >= 0)
	close (f);
      if (flag_atime_preserve)
	utime (p, &restore_times);
      return;
    }

#ifdef S_ISLNK
  else if (S_ISLNK (hstat.st_mode))
    {
      int size;
      char *buf = (char *) alloca (PATH_MAX + 1);

      size = readlink (p, buf, PATH_MAX + 1);
      if (size < 0)
	goto badperror;
      buf[size] = '\0';
      if (size >= NAMSIZ)
	__tar_write_long (buf, LF_LONGLINK);
      __tar_assign_string (&current_link_name, buf);

      hstat.st_size = 0;	/* force 0 size on symlink */
      header = __tar_start_header (p, &hstat);
      if (header == NULL)
	{
	  critical_error = 1;
	  goto badfile;
	}
      strncpy (header->header.arch_linkname, buf, NAMSIZ);
      header->header.arch_linkname[NAMSIZ - 1] = '\0';
      header->header.linkflag = LF_SYMLINK;
      __tar_finish_header (header);	/* nothing more to do to it */
      if (flag_remove_files)
	{
	  if (unlink (p) == -1)
	    ERROR ((0, errno, _("Cannot remove %s"), p));
	}
      return;
    }
#endif /* S_ISLNK */

  else if (S_ISDIR (hstat.st_mode))
    {
      register DIR *dirp;
      register struct dirent *d;
      char *namebuf;
      int buflen;
      register int len;
      int our_device = hstat.st_dev;

      /* Build new prototype name.  */

      len = strlen (p);
      buflen = len + NAMSIZ;
      namebuf = tar_xmalloc ((size_t) (buflen + 1));
      strncpy (namebuf, p, (size_t) buflen);
      while (len >= 1 && namebuf[len - 1] == '/')
	len--;			/* delete trailing slashes */
      namebuf[len++] = '/';	/* now add exactly one back */
      namebuf[len] = '\0';	/* make sure null-terminated */

      /* Output directory header record with permissions FIXME, do this
	 AFTER files, to avoid R/O dir problems?  If old archive format,
	 don't write record at all.  */

      if (!flag_oldarch)
	{
	  hstat.st_size = 0;	/* force 0 size on dir */

	  /* If people could really read standard archives, this should
	     be: (FIXME)

	     header = __tar_start_header (flag_standard ? p : namebuf, &hstat);

	     but since they'd interpret LF_DIR records as regular files,
	     we'd better put the / on the name.  */

	  header = __tar_start_header (namebuf, &hstat);
	  if (header == NULL)
	    {
	      critical_error = 1;
	      goto badfile;	/* eg name too long */
	    }

	  if (flag_gnudump)
	    header->header.linkflag = LF_DUMPDIR;
	  else if (flag_standard)
	    header->header.linkflag = LF_DIR;

	  /* If we're gnudumping, we aren't done yet so don't close it.  */

	  if (!flag_gnudump)
	    __tar_finish_header (header);	/* done with directory header */
	}

      if (flag_gnudump && gnu_list_name->dir_contents)
	{
	  int sizeleft;
	  int totsize;
	  int bufsize;
	  union record *start;
	  int count;
	  char *buf, *p_buf;

	  buf = gnu_list_name->dir_contents; /* FOO */
	  totsize = 0;
	  for (p_buf = buf; p_buf && *p_buf;)
	    {
	      int tmp;

	      tmp = strlen (p_buf) + 1;
	      totsize += tmp;
	      p_buf += tmp;
	    }
	  totsize++;
	  __tar_to_oct ((long) totsize, 1 + 12, header->header.size);
	  __tar_finish_header (header);
	  p_buf = buf;
	  sizeleft = totsize;
	  while (sizeleft > 0)
	    {
	      if (flag_multivol)
		{
		  __tar_assign_string (&save_name, p);
		  save_sizeleft = sizeleft;
		  save_totsize = totsize;
		}
	      start = __tar_findrec ();
	      bufsize = __tar_endofrecs ()->charptr - start->charptr;
	      if (sizeleft < bufsize)
		{
		  bufsize = sizeleft;
		  count = bufsize % RECORDSIZE;
		  if (count)
		    memset (start->charptr + sizeleft, 0,
			   (size_t) (RECORDSIZE - count));
		}
	      memcpy (start->charptr, p_buf, (size_t) bufsize);
	      sizeleft -= bufsize;
	      p_buf += bufsize;
	      __tar_userec (start + (bufsize - 1) / RECORDSIZE);
	    }
	  if (flag_multivol)
	    __tar_assign_string (&save_name, NULL);
	  if (flag_atime_preserve)
	    utime (p, &restore_times);
	  return;
	}

      /* Now output all the files in the directory.  */

#if 0
      if (flag_dironly)
	return;			/* unless the cmdline said not to */
#endif

      /* See if we are crossing from one file system to another, and
	 avoid doing so if the user only wants to dump one file system.  */

      if (flag_local_filesys && !toplevel && curdev != hstat.st_dev)
	{
	  if (flag_verbose)
	    WARN ((0, 0, _("%s: On a different filesystem; not dumped"), p));
	  return;
	}

      errno = 0;
      dirp = opendir (p);
      if (!dirp)
	{
	  ERROR ((0, errno, _("Cannot open directory %s"), p));
	  return;
	}

      /* Hack to remove "./" from the front of all the file names.  */

      if (len == 2 && namebuf[0] == '.' && namebuf[1] == '/')
	len = 0;

      /* Should speed this up by cd-ing into the dir, FIXME.  */

      while (d = readdir (dirp), d)
	{

	  /* Skip `.' and `..'.  */

	  if (__tar_is_dot_or_dotdot (d->d_name))
	    continue;

	  if ((int) NAMLEN (d) + len >= buflen)
	    {
	      buflen = len + NAMLEN (d);
	      namebuf = (char *) tar_realloc (namebuf, (size_t) (buflen + 1));
#if 0
	      namebuf[len] = '\0';
	      ERROR ((0, 0, _("File name %s%s too long"), namebuf, d->d_name));
	      continue;
#endif
	    }
	  strcpy (namebuf + len, d->d_name);
	  if (flag_exclude && __tar_check_exclude (namebuf))
	    continue;
	  __tar_dump_file (namebuf, our_device, 0);
	}

      closedir (dirp);
      free (namebuf);
      if (flag_atime_preserve)
	utime (p, &restore_times);
      return;
    }

#ifdef S_ISCHR
  else if (S_ISCHR (hstat.st_mode))
    type = LF_CHR;
#endif

#ifdef S_ISBLK
  else if (S_ISBLK (hstat.st_mode))
    type = LF_BLK;
#endif

  /* Avoid screwy apollo lossage where S_IFIFO == S_IFSOCK.  */

#if (_ISP__M68K == 0) && (_ISP__A88K == 0) && defined(S_ISFIFO)
  else if (S_ISFIFO (hstat.st_mode))
    type = LF_FIFO;
#endif

#ifdef S_ISSOCK
  else if (S_ISSOCK (hstat.st_mode))
    type = LF_FIFO;
#endif

  else
    goto unknown;

  if (!flag_standard)
    goto unknown;

  hstat.st_size = 0;		/* force 0 size */
  header = __tar_start_header (p, &hstat);
  if (header == NULL)
    {
      critical_error = 1;
      goto badfile;		/* eg name too long */
    }

  header->header.linkflag = type;

#if defined(S_IFBLK) || defined(S_IFCHR)
  if (type != LF_FIFO)
    {
      __tar_to_oct ((long) major (hstat.st_rdev), 8,
	      header->header.devmajor);
      __tar_to_oct ((long) minor (hstat.st_rdev), 8,
	      header->header.devminor);
    }
#endif

  __tar_finish_header (header);
  if (flag_remove_files)
    {
      if (unlink (p) == -1)
	ERROR ((0, errno, _("Cannot remove %s"), p));
    }
  return;

unknown:
  ERROR ((0, 0, _("%s: Unknown file type; file ignored"), p));
}
