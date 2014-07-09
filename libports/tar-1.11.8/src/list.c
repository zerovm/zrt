/* List a tar archive.
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

/* List a tar archive.
   Also includes support routines for reading a tar archive.
   Written 26 Aug 1985 by John Gilmore.  */

#include "system.h"

#include <ctype.h>
#include <time.h>

#define	ISODIGIT(Char) \
  ((unsigned char) (Char) >= '0' && (unsigned char) (Char) <= '7')
#define ISSPACE(Char) (ISASCII (Char) && isspace (Char))

#include "tar.h"

static void __tar_demode __P ((unsigned, char *));

union record *head;		/* points to current archive header */
struct stat hstat;		/* stat struct corresponding */
int head_standard;		/* tape header is in ANSI format */

/*-----------------------------------.
| Main loop for reading an archive.  |
`-----------------------------------*/

void
__tar_read_and (void (*do_something) ())
{
  int status = 3;		/* initial status at start of archive */
  int prev_status;
  char save_linkflag;

  __tar_name_gather ();		/* gather all the names */
  __tar_open_tar_archive (1);		/* open for reading */

  while (1)
    {
      prev_status = status;
      status = __tar_read_header ();
      switch (status)
	{

	case 1:

	  /* Valid header.  We should decode next field (mode) first.
	     Ensure incoming names are null terminated.  */

	  if (!__tar_name_match (current_file_name)
	      || (flag_new_files && hstat.st_mtime < new_time)
	      || (flag_exclude && __tar_check_exclude (current_file_name)))
	    {

	      int isextended = 0;

	      if (head->header.linkflag == LF_VOLHDR
		  || head->header.linkflag == LF_MULTIVOL
		  || head->header.linkflag == LF_NAMES)
		{
		  (*do_something) ();
		  continue;
		}
	      if (flag_show_omitted_dirs && head->header.linkflag == LF_DIR)
		WARN ((0, 0, _("Omitting %s"), current_file_name));

	      /* Skip past it in the archive.  */

	      if (head->header.isextended)
		isextended = 1;
	      save_linkflag = head->header.linkflag;
	      __tar_userec (head);
	      if (isextended)
		{
#if 0
		  register union record *exhdr;

		  while (1)
		    {
		      exhdr = __tar_findrec ();
		      if (!exhdr->ext_hdr.isextended)
			{
			  __tar_userec (exhdr);
			  break;
			}
		    }
		  __tar_userec (exhdr);
#endif
		  __tar_skip_extended_headers ();
		}

	      /* Skip to the next header on the archive.  */

	      if (save_linkflag != LF_DIR)
		__tar_skip_file ((long) hstat.st_size);
	      continue;
	    }

	  (*do_something) ();
	  continue;

	  /* If the previous header was good, tell them that we are
	     skipping bad ones.  */

	case 0:			/* invalid header */
	  __tar_userec (head);
	  switch (prev_status)
	    {
	    case 3:		/* error on first record */
	      WARN ((0, 0, _("Hmm, this doesn't look like a tar archive")));
	      /* Fall through.  */

	    case 2:		/* error after record of zeroes */
	    case 1:		/* error after header rec */
	      WARN ((0, 0, _("Skipping to next file header")));
	    case 0:		/* error after error */
	      break;
	    }
	  continue;

	case 2:			/* record of zeroes */
	  __tar_userec (head);
	  status = prev_status;	/* if error after 0's */
	  if (flag_ignorez)
	    continue;
	  /* Fall through.  */

	case EOF:		/* end of archive */
	  break;
	}
      break;
    };

  __tar_restore_saved_dir_info ();
  close_tar_archive ();
  __tar_names_notfound ();		/* print names not found */
}

/*----------------------------------------------.
| Print a header record, based on tar options.  |
`----------------------------------------------*/

void
__tar_list_archive (void)
{
  int isextended = 0;		/* flag to remember if head is extended */

  /* Save the record.  */

  __tar_saverec (&head);

  /* Print the header record.  */

  if (flag_verbose)
    {
      if (flag_verbose > 1)
	__tar_decode_header (head, &hstat, &head_standard, 0);
      __tar_print_header ();
    }

  if (flag_gnudump && head->header.linkflag == LF_DUMPDIR)
    {
      size_t size, written, check;
      char *data;

      __tar_userec (head);
      if (flag_multivol)
	{
	  __tar_assign_string (&save_name, current_file_name);
	  save_totsize = hstat.st_size;
	}
      for (size = hstat.st_size; size > 0; size -= written)
	{
	  if (flag_multivol)
	    save_sizeleft = size;
	  data = __tar_findrec ()->charptr;
	  if (data == NULL)
	    {
	      ERROR ((0, 0, _("EOF in archive file")));
	      break;
	    }
	  written = __tar_endofrecs ()->charptr - data;
	  if (written > size)
	    written = size;
	  errno = 0;
	  check = fwrite (data, sizeof (char), written, stdlis);
	  __tar_userec ((union record *) (data + written - 1));
	  if (check != written)
	    {
	      ERROR ((0, errno, _("Only wrote %ld of %ld bytes to file %s"),
		      check, written, current_file_name));
	      __tar_skip_file ((long) (size - written));
	      break;
	    }
	}
      if (flag_multivol)
	__tar_assign_string (&save_name, NULL);
      __tar_saverec (NULL);		/* unsave it */
      fputc ('\n', stdlis);
      fflush (stdlis);
      return;

    }
  __tar_saverec ((union record **) 0);	/* unsave it */

  /* Check to see if we have an extended header to skip over also.  */

  if (head->header.isextended)
    isextended = 1;

  /* Skip past the header in the archive.  */

  __tar_userec (head);

  /* If we needed to skip any extended headers, do so now, by reading
     extended headers and skipping past them in the archive.  */

  if (isextended)
    {
#if 0
      register union record *exhdr;

      while (1)
	{
	  exhdr = __tar_findrec ();

	  if (!exhdr->ext_hdr.isextended)
	    {
	      __tar_userec (exhdr);
	      break;
	    }
	  __tar_userec (exhdr);
	}
#endif
      __tar_skip_extended_headers ();
    }

  if (flag_multivol)
    __tar_assign_string (&save_name, current_file_name);

  /* Skip to the next header on the archive.  */

  __tar_skip_file ((long) hstat.st_size);

  if (flag_multivol)
    __tar_assign_string (&save_name, NULL);
}

/*-------------------------------------------------------------------------.
| Read a record that's supposed to be a header record.  Return its address |
| in "head", and if it is good, the file's size in hstat.st_size.	   |
| 									   |
| Return 1 for success, 0 if the checksum is bad, EOF on eof, 2 for a	   |
| record full of zeros (EOF marker).					   |
| 									   |
| You must always __tar_userec(head) to skip past the header which this routine  |
| reads.								   |
`-------------------------------------------------------------------------*/

/* The standard BSD tar sources create the checksum by adding up the
   bytes in the header as type char.  I think the type char was unsigned
   on the PDP-11, but it's signed on the Next and Sun.  It looks like the
   sources to BSD tar were never changed to compute the checksum
   currectly, so both the Sun and Next add the bytes of the header as
   signed chars.  This doesn't cause a problem until you get a file with
   a name containing characters with the high bit set.  So __tar_read_header
   computes two checksums -- signed and unsigned.  */

int
__tar_read_header (void)
{
  register int i;
  register long sum, signed_sum, recsum;
  register char *p;
  register union record *header;
  char **longp;
  char *bp, *data;
  int size, written;
  static char *next_long_name, *next_long_link;

  while (1)
    {
      header = __tar_findrec ();
      head = header;		/* this is our current header */
      if (!header)
	return EOF;

      recsum = __tar_from_oct (8, header->header.chksum);

      sum = 0;
      signed_sum = 0;
      p = header->charptr;
      for (i = sizeof (*header); --i >= 0;)
	{

	  /* We can't use unsigned char here because of old compilers,
	     e.g. V7.  */

	  sum += 0xFF & *p;
	  signed_sum += *p++;
	}

      /* Adjust checksum to count the "chksum" field as blanks.  */

      for (i = sizeof (header->header.chksum); --i >= 0;)
	{
	  sum -= 0xFF & header->header.chksum[i];
	  signed_sum -= (char) header->header.chksum[i];
	}
      sum += ' ' * sizeof header->header.chksum;
      signed_sum += ' ' * sizeof header->header.chksum;

      if (sum == 8 * ' ')
	{

	  /* This is a zeroed record...whole record is 0's except for the 8
	     blanks we faked for the checksum field.  */

	  return 2;
	}

      if (sum != recsum && signed_sum != recsum)
	return 0;

      /* Good record.  Decode file size and return.  */

      if (header->header.linkflag == LF_LINK)
	hstat.st_size = 0;	/* links 0 size on tape */
      else
	hstat.st_size = __tar_from_oct (1 + 12, header->header.size);

      header->header.arch_name[NAMSIZ - 1] = '\0';
      if (header->header.linkflag == LF_LONGNAME
	  || header->header.linkflag == LF_LONGLINK)
	{
	  longp = ((header->header.linkflag == LF_LONGNAME)
		   ? &next_long_name
		   : &next_long_link);

	  __tar_userec (header);
	  if (*longp)
	    free (*longp);
	  bp = *longp = (char *) __tar_ck_malloc ((size_t) hstat.st_size);

	  for (size = hstat.st_size; size > 0; size -= written)
	    {
	      data = __tar_findrec ()->charptr;
	      if (data == NULL)
		{
		  ERROR ((0, 0, _("Unexpected EOF on archive file")));
		  break;
		}
	      written = __tar_endofrecs ()->charptr - data;
	      if (written > size)
		written = size;

	      memcpy (bp, data, (size_t) written);
	      bp += written;
	      __tar_userec ((union record *) (data + written - 1));
	    }

	  /* Loop!  */

	}
      else
	{
	  __tar_assign_string (&current_file_name, (next_long_name ? next_long_name
					      : head->header.arch_name));
	  __tar_assign_string (&current_link_name, (next_long_link ? next_long_link
					      : head->header.arch_linkname));
	  next_long_link = next_long_name = 0;
	  return 1;
	}
    }
}

/*-------------------------------------------------------------------------.
| Decode things from a file header record into a "struct stat".  Also set  |
| "*stdp" to !=0 or ==0 depending whether header record is "Unix Standard" |
| tar format or regular old tar format.					   |
| 									   |
| __tar_read_header() has already decoded the checksum and length, so we don't.  |
| 									   |
| If wantug != 0, we want the uid/group info decoded from Unix Standard	   |
| tapes (for extraction).  If == 0, we are just printing anyway, so save   |
| time.									   |
| 									   |
| __tar_decode_header should NOT be called twice for the same record, since the  |
| two calls might use different "wantug" values and thus might end up with |
| different uid/gid for the two calls.  If anybody wants the uid/gid they  |
| should decode it first, and other callers should decode it without	   |
| uid/gid before calling a routine, e.g. __tar_print_header, that assumes	   |
| decoded data.								   |
`-------------------------------------------------------------------------*/

void
__tar_decode_header (register union record *header, register struct stat *st,
	       int *stdp, int wantug)
{
  st->st_mode = __tar_from_oct (8, header->header.mode);
  st->st_mode &= 07777;
  st->st_mtime = __tar_from_oct (1 + 12, header->header.mtime);
  if (flag_gnudump)
    {
      st->st_atime = __tar_from_oct (1 + 12, header->header.atime);
      st->st_ctime = __tar_from_oct (1 + 12, header->header.ctime);
    }

  if (strcmp (header->header.magic, TMAGIC) == 0)
    {

      /* Unix Standard tar archive.  */

      *stdp = 1;
      if (wantug)
	{
#ifdef NONAMES
	  st->st_uid = __tar_from_oct (8, header->header.uid);
	  st->st_gid = __tar_from_oct (8, header->header.gid);
#else
	  st->st_uid =
	    (*header->header.uname
	     ? __tar_finduid (header->header.uname)
	     : __tar_from_oct (8, header->header.uid));
	  st->st_gid =
	    (*header->header.gname
	     ? __tar_findgid (header->header.gname)
	     : __tar_from_oct (8, header->header.gid));
#endif
	}
#if defined(S_IFBLK) || defined(S_IFCHR)
      switch (header->header.linkflag)
	{
	case LF_BLK:
	case LF_CHR:
	  st->st_rdev = makedev (__tar_from_oct (8, header->header.devmajor),
				 __tar_from_oct (8, header->header.devminor));
	}
#endif
    }
  else
    {

      /* Old fashioned tar archive.  */

      *stdp = 0;
      st->st_uid = __tar_from_oct (8, header->header.uid);
      st->st_gid = __tar_from_oct (8, header->header.gid);
      st->st_rdev = 0;
    }
}

/*------------------------------------------------------------------------.
| Quick and dirty octal conversion.  Result is -1 if the field is invalid |
| (all blank, or nonoctal).						  |
`------------------------------------------------------------------------*/

long
__tar_from_oct (register int digs, register char *where)
{
  register long value;

  while (ISSPACE (*where))
    {				/* skip spaces */
      where++;
      if (--digs <= 0)
	return -1;		/* all blank field */
    }
  value = 0;
  while (digs > 0 && ISODIGIT (*where))
    {

      /* Scan til nonoctal.  */

      value = (value << 3) | (*where++ - '0');
      --digs;
    }

  if (digs > 0 && *where && !ISSPACE (*where))
    return -1;			/* ended on non-space/nul */

  return value;
}

/*-------------------------------------------------------------------------.
| Actually print it.							   |
| 									   |
| Plain and fancy file header block logging.  Non-verbose just prints the  |
| name, e.g. for "tar t" or "tar x".  This should just contain file names, |
| so it can be fed back into tar with xargs or the "-T" option.  The	   |
| verbose option can give a bunch of info, one line per file.  I doubt	   |
| anybody tries to parse its format, or if they do, they shouldn't.  Unix  |
| tar is pretty random here anyway.					   |
`-------------------------------------------------------------------------*/

/* Note that __tar_print_header uses the globals <head>, <hstat>, and
   <head_standard>, which must be set up in advance.  This is not very
   clean and should be cleaned up.  FIXME.  */

#define	UGSWIDTH	18	/* min width of User, group, size */
/* UGSWIDTH of 18 means that with user and group names <= 8 chars the columns
   never shift during the listing.  */
#define	DATEWIDTH	19	/* last mod date */
static int ugswidth = UGSWIDTH;	/* max width encountered so far */

void
__tar_print_header (void)
{
  char modes[11];
  char *timestamp;
  char uform[11], gform[11];	/* these hold formatted ints */
  char *user, *group;
  char size[24];		/* holds a formatted long or maj, min */
  time_t longie;		/* to make ctime() call portable */
  int pad;
  char *name;

  if (flag_sayblock)
    fprintf (stdlis, _("rec %10ld: "), baserec + (ar_record - ar_block));
#if 0
  annofile (stdlis, (char *) NULL);
#endif

  if (flag_verbose <= 1)
    {

      /* Just the fax, mam.  */

      char *quoted_name = __tar_quote_copy_string (current_file_name);

      if (quoted_name)
	{
	  fprintf (stdlis, "%s\n", quoted_name);
	  free (quoted_name);
	}
      else
	fprintf (stdlis, "%s\n", current_file_name);
    }
  else
    {

      /* File type and modes.  */

      modes[0] = '?';
      switch (head->header.linkflag)
	{
	case LF_VOLHDR:
	  modes[0] = 'V';
	  break;

	case LF_MULTIVOL:
	  modes[0] = 'M';
	  break;

	case LF_NAMES:
	  modes[0] = 'N';
	  break;

	case LF_LONGNAME:
	case LF_LONGLINK:
	  ERROR ((0, 0, _("Visible longname error")));
	  break;

	case LF_SPARSE:
	case LF_NORMAL:
	case LF_OLDNORMAL:
	case LF_LINK:
	  modes[0] = '-';
	  if (current_file_name[strlen (current_file_name) - 1] == '/')
	    modes[0] = 'd';
	  break;
	case LF_DUMPDIR:
	  modes[0] = 'd';
	  break;
	case LF_DIR:
	  modes[0] = 'd';
	  break;
	case LF_SYMLINK:
	  modes[0] = 'l';
	  break;
	case LF_BLK:
	  modes[0] = 'b';
	  break;
	case LF_CHR:
	  modes[0] = 'c';
	  break;
	case LF_FIFO:
	  modes[0] = 'p';
	  break;
	case LF_CONTIG:
	  modes[0] = 'C';
	  break;
	}

      __tar_demode ((unsigned) hstat.st_mode, modes + 1);

      /* Timestamp.  */

      longie = hstat.st_mtime;
      timestamp = ctime (&longie);
      timestamp[16] = '\0';
      timestamp[24] = '\0';

      /* User and group names.  */

      if (*head->header.uname && head_standard)
	{
	  user = head->header.uname;
	}
      else
	{
	  user = uform;
	  sprintf (uform, "%ld", __tar_from_oct (8, head->header.uid));
	}
      if (*head->header.gname && head_standard)
	{
	  group = head->header.gname;
	}
      else
	{
	  group = gform;
	  sprintf (gform, "%ld", __tar_from_oct (8, head->header.gid));
	}

      /* Format the file size or major/minor device numbers.  */

      switch (head->header.linkflag)
	{
#if defined(S_IFBLK) || defined(S_IFCHR)
	case LF_CHR:
	case LF_BLK:
	  sprintf (size, "%d,%d",
		   major (hstat.st_rdev), minor (hstat.st_rdev));
	  break;
#endif
	case LF_SPARSE:
	  sprintf (size, "%ld", __tar_from_oct (1 + 12, head->header.realsize));
	  break;
	default:
	  sprintf (size, "%ld", (long) hstat.st_size);
	}

      /* Figure out padding and print the whole line.  */

      pad = strlen (user) + strlen (group) + strlen (size) + 1;
      if (pad > ugswidth)
	ugswidth = pad;

      fprintf (stdlis, "%s %s/%s %*s%s %s %s",
	       modes, user, group, ugswidth - pad, "",
	       size, timestamp + 4, timestamp + 20);

      name = __tar_quote_copy_string (current_file_name);
      if (name)
	{
	  fprintf (stdlis, " %s", name);
	  free (name);
	}
      else
	fprintf (stdlis, " %s", current_file_name);

      switch (head->header.linkflag)
	{
	case LF_SYMLINK:
	  name = __tar_quote_copy_string (current_link_name);
	  if (name)
	    {
	      fprintf (stdlis, " -> %s\n", name);
	      free (name);
	    }
	  else
	    fprintf (stdlis, " -> %s\n", current_link_name);
	  break;

	case LF_LINK:
	  name = __tar_quote_copy_string (current_link_name);
	  if (name)
	    {
	      fprintf (stdlis, _(" link to %s\n"), name);
	      free (name);
	    }
	  else
	    fprintf (stdlis, _(" link to %s\n"), current_link_name);
	  break;

	default:
	  fprintf (stdlis, _(" unknown file type `%c'\n"),
		   head->header.linkflag);
	  break;

	case LF_OLDNORMAL:
	case LF_NORMAL:
	case LF_SPARSE:
	case LF_CHR:
	case LF_BLK:
	case LF_DIR:
	case LF_FIFO:
	case LF_CONTIG:
	case LF_DUMPDIR:
	  putc ('\n', stdlis);
	  break;

	case LF_VOLHDR:
	  fprintf (stdlis, _("--Volume Header--\n"));
	  break;

	case LF_MULTIVOL:
	  fprintf (stdlis, _("--Continued at byte %ld--\n"),
		   __tar_from_oct (1 + 12, head->header.offset));
	  break;

	case LF_NAMES:
	  fprintf (stdlis, _("--Mangled file names--\n"));
	  break;
	}
    }
  fflush (stdlis);
}

/*--------------------------------------------------------------.
| Print a similar line when we make a directory automatically.  |
`--------------------------------------------------------------*/

void
__tar_pr_mkdir (char *pathname, int length, int mode)
{
  char modes[11];
  char *name;

  if (flag_verbose > 1)
    {

      /* File type and modes.  */

      modes[0] = 'd';
      __tar_demode ((unsigned) mode, modes + 1);

      if (flag_sayblock)
	fprintf (stdlis, _("rec %10ld: "), baserec + (ar_record - ar_block));
#if 0
      annofile (stdlis, (char *) NULL);
#endif
      name = __tar_quote_copy_string (pathname);
      if (!name)
	name = pathname;
      fprintf (stdlis, "%s %*s %.*s\n", modes, ugswidth + DATEWIDTH,
	       _("Creating directory:"), length, pathname);
      if (name != pathname)
	free (name);
    }
}

/*-----------------------------------------------------------.
| Skip over <size> bytes of data in records in the archive.  |
`-----------------------------------------------------------*/

void
__tar_skip_file (register long size)
{
  union record *x;

  if (flag_multivol)
    {
      save_totsize = size;
      save_sizeleft = size;
    }

  while (size > 0)
    {
      x = __tar_findrec ();
      if (x == NULL)
	ERROR ((TAREXIT_FAILURE, 0, _("Unexpected EOF on archive file")));

      __tar_userec (x);
      size -= RECORDSIZE;
      if (flag_multivol)
	save_sizeleft -= RECORDSIZE;
    }
}

/*---.
| ?  |
`---*/

void
__tar_skip_extended_headers (void)
{
  register union record *exhdr;

  while (1)
    {
      exhdr = __tar_findrec ();
      if (!exhdr->ext_hdr.isextended)
	{
	  __tar_userec (exhdr);
	  break;
	}
      __tar_userec (exhdr);
    }
}

/*--------------------------------------------------------------------.
| Decode the mode string from a stat entry into a 9-char string and a |
| null.								      |
`--------------------------------------------------------------------*/

static void
__tar_demode (register unsigned mode, register char *string)
{
  register unsigned mask;
  register const char *rwx = "rwxrwxrwx";

  for (mask = 0400; mask != 0; mask >>= 1)
    {
      if (mode & mask)
	*string++ = *rwx++;
      else
	{
	  *string++ = '-';
	  rwx++;
	}
    }

  if (mode & S_ISUID)
    if (string[-7] == 'x')
      string[-7] = 's';
    else
      string[-7] = 'S';
  if (mode & S_ISGID)
    if (string[-4] == 'x')
      string[-4] = 's';
    else
      string[-4] = 'S';
  if (mode & S_ISVTX)
    if (string[-1] == 'x')
      string[-1] = 't';
    else
      string[-1] = 'T';
  *string = '\0';
}
