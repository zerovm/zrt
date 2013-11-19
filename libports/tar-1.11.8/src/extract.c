/* Extract files from a tar archive.
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

/* Extract files from a tar archive.
   Written 19 Nov 1985 by John Gilmore.  */

#include "system.h"

#include <time.h>
time_t time ();

/* We need the #define's even though we don't use them. */
#if NO_OPEN3
# include "open3.h"
#endif

/* Simulated 3-argument open for systems that don't have it */
#if EMUL_OPEN3
# include "open3.h"
#endif

#if HAVE_UTIME_H
# include <utime.h>
#else

struct utimbuf
  {
    long actime;
    long modtime;
  };

#endif

#include "tar.h"

extern union record *head;	/* points to current tape header */
extern struct stat hstat;	/* stat struct corresponding */
extern int head_standard;	/* tape header is in ANSI format */

static void extract_sparse_file __P ((int, long *, long, char *));
static int make_dirs __P ((char *));

static time_t now = 0;		/* current time */
static we_are_root = 0;		/* true if our effective uid == 0 */
static int notumask = ~0;	/* masks out bits user doesn't want */

#if 0
/* "Scratch" space to store the information about a sparse file before
   writing the info into the header or extended header.  */
struct sp_array       *sparsearray;

/* Number of elts storable in the sparsearray.  */
int   sp_array_size = 10;
#endif

struct saved_dir_info
  {
    char *path;
    int mode;
    int atime;
    int mtime;
    uid_t uid;
    gid_t gid;
    struct saved_dir_info *next;
  };

struct saved_dir_info *saved_dir_info_head;

/*--------------------------.
| Set up to extract files.  |
`--------------------------*/

void
extr_init (void)
{
  int ourmask;

  now = time ((time_t *) 0);
  if (geteuid () == 0)
    we_are_root = 1;

  /* We need to know our umask.  But if flag_use_protection is set, leave
     our kernel umask at 0, and our "notumask" at ~0.  */

  ourmask = umask (0);		/* read it */
  if (!flag_use_protection)
    {
      umask (ourmask);		/* set it back how it was */
      notumask = ~ourmask;	/* make umask override permissions */
    }
}

/*----------------------------------.
| Extract a file from the archive.  |
`----------------------------------*/

void
extract_archive (void)
{
  register char *data;
  int fd, check, namelen, written, openflag;
  long size;
  struct utimbuf acc_upd_times;
  register int skipcrud;
  register int i;
#if 0
  int sparse_ind = 0;
#endif
  union record *exhdr;
  struct saved_dir_info *tmp;
#if 0
  int end_nulls;
#endif
#ifdef O_CTG
  /* FIXME: Dirty kludge so the code compile on Masscomp's.  I do not know
     what longname is supposed to be.  NULL makes it unused.  */
  char *longname = NULL;
#endif

#define CURRENT_FILE_NAME (skipcrud + current_file_name)

  saverec (&head);		/* make sure it sticks around */
  userec (head);		/* and go past it in the archive */
  decode_header (head, &hstat, &head_standard, 1);

  if (flag_confirm && !confirm ("extract", current_file_name))
    {
      if (head->header.isextended)
	skip_extended_headers ();
      skip_file ((long) hstat.st_size);
      saverec ((union record **) 0);
      return;
    }

  /* Print the record from `head' and `hstat'.  */

  if (flag_verbose)
    print_header ();

  /* Check for fully specified pathnames and other atrocities.

     Note, we can't just make a pointer to the new file name, since
     saverec() might move the header and adjust "head".  We have to start
     from "head" every time we want to touch the header record.  */

  skipcrud = 0;
  while (!flag_absolute_names && CURRENT_FILE_NAME[0] == '/')
    {
      static int warned_once = 0;

      skipcrud++;		/* force relative path */
      if (!warned_once++)
	WARN ((0, 0, _("\
Removing leading / from absolute path names in the archive")));
    }

  switch (head->header.linkflag)
    {

    default:
      WARN ((0, 0,
	     _("Unknown file type '%c' for %s, extracted as normal file"),
	     head->header.linkflag, CURRENT_FILE_NAME));

      /* Fall through.  */

      /* JK - What we want to do if the file is sparse is loop through
	 the array of sparse structures in the header and read in and
	 translate the character strings representing 1) the offset at
	 which to write and 2) how many bytes to write into numbers,
	 which we store into the scratch array, "sparsearray".  This
	 array makes our life easier the same way it did in creating the
	 tar file that had to deal with a sparse file.

	 After we read in the first five (at most) sparse structures, we
	 check to see if the file has an extended header, i.e., if more
	 sparse structures are needed to describe the contents of the new
	 file.  If so, we read in the extended headers and continue to
	 store their contents into the sparsearray.  */

    case LF_SPARSE:
      sp_array_size = 10;
      sparsearray = (struct sp_array *) tar_xmalloc (sp_array_size * sizeof (struct sp_array));
      for (i = 0; i < SPARSE_IN_HDR; i++)
	{
	  sparsearray[i].offset =
	    from_oct (1 + 12, head->header.sp[i].offset);
	  sparsearray[i].numbytes =
	    from_oct (1 + 12, head->header.sp[i].numbytes);
	  if (!sparsearray[i].numbytes)
	    break;
	}
#if 0
      end_nulls = from_oct (1 + 12, head->header.ending_blanks);
#endif

      if (head->header.isextended)
	{

	  /* Read in the list of extended headers and translate them into
	     the sparsearray as before.  */

	  /* static */ int ind = SPARSE_IN_HDR;

	  while (1)
	    {

	      exhdr = findrec ();
	      for (i = 0; i < SPARSE_EXT_HDR; i++)
		{

		  if (i + ind > sp_array_size - 1)
		    {

		      /* Realloc the scratch area since we've run out of
			 room.  */

		      sp_array_size *= 2;
		      sparsearray = (struct sp_array *)
			tar_realloc (sparsearray,
				  sp_array_size * (sizeof (struct sp_array)));
		    }
		  /* Compare to 0, or use !(int)..., for Pyramid's dumb
		     compiler.  */
		  if (exhdr->ext_hdr.sp[i].numbytes == 0)
		    break;
		  sparsearray[i + ind].offset =
		    from_oct (1 + 12, exhdr->ext_hdr.sp[i].offset);
		  sparsearray[i + ind].numbytes =
		    from_oct (1 + 12, exhdr->ext_hdr.sp[i].numbytes);
		}
	      if (!exhdr->ext_hdr.isextended)
		break;
	      else
		{
		  ind += SPARSE_EXT_HDR;
		  userec (exhdr);
		}
	    }
	  userec (exhdr);
	}
      /* Fall through.  */

    case LF_OLDNORMAL:
    case LF_NORMAL:
    case LF_CONTIG:

      /* Appears to be a file.  See if it's really a directory.  */

      namelen = strlen (CURRENT_FILE_NAME) - 1;
      if (CURRENT_FILE_NAME[namelen] == '/')
	goto really_dir;

      /* FIXME, deal with protection issues.  */

    again_file:
      openflag = (flag_keep ?
		  O_BINARY | O_NDELAY | O_WRONLY | O_CREAT | O_EXCL :
		  O_BINARY | O_NDELAY | O_WRONLY | O_CREAT | O_TRUNC)
	| ((head->header.linkflag == LF_SPARSE) ? 0 : O_APPEND);

      /* JK - The last | is a kludge to solve the problem the O_APPEND
	 flag causes with files we are trying to make sparse: when a file
	 is opened with O_APPEND, it writes to the last place that
	 something was written, thereby ignoring any lseeks that we have
	 done.  We add this extra condition to make it able to lseek when
	 a file is sparse, i.e., we don't open the new file with this
	 flag.  (Grump -- this bug caused me to waste a good deal of
	 time, I might add)  */

      if (flag_exstdout)
	{
	  fd = 1;
	  goto extract_file;
	}
#ifdef O_CTG
      /* Contiguous files (on the Masscomp) have to specify the size in
	 the open call that creates them.  */

      if (head->header.linkflag == LF_CONTIG)
	fd = open ((longname ? longname : head->header.name) + skipcrud,
		   openflag | O_CTG, hstat.st_mode, hstat.st_size);
      else
#endif
	{
#ifdef NO_OPEN3
	  /* On raw V7 we won't let them specify -k (flag_keep), but we just
	     bull ahead and create the files.  */

	  fd = creat ((longname ? longname : head->header.name) + skipcrud,
		      hstat.st_mode);
#else
	  /* With 3-arg open(), we can do this up right.  */

	  fd = open (CURRENT_FILE_NAME, openflag, hstat.st_mode);
#endif
	}

      if (fd < 0)
	{
	  if (make_dirs (CURRENT_FILE_NAME))
	    goto again_file;

	  ERROR ((0, errno, _("Could not create file %s"), CURRENT_FILE_NAME));
	  if (head->header.isextended)
	    skip_extended_headers ();
	  skip_file ((long) hstat.st_size);
	  goto quit;
	}

    extract_file:
      if (head->header.linkflag == LF_SPARSE)
	{
	  char *name;
	  int namelen_bis;

	  /* Kludge alert.  NAME is assigned to header.name because
	     during the extraction, the space that contains the header
	     will get scribbled on, and the name will get munged, so any
	     error messages that happen to contain the filename will look
	     REAL interesting unless we do this.  */

	  namelen_bis = strlen (CURRENT_FILE_NAME) + 1;
	  name = (char *) tar_xmalloc ((sizeof (char)) * namelen_bis);
	  memcpy (name, CURRENT_FILE_NAME, (size_t) namelen_bis);
	  size = hstat.st_size;
	  extract_sparse_file (fd, &size, hstat.st_size, name);
	}
      else
	for (size = hstat.st_size;
	     size > 0;
	     size -= written)
	  {
#if 0
	    long offset, numbytes;
#endif
	    if (flag_multivol)
	      {
		assign_string (&save_name, current_file_name);
		save_totsize = hstat.st_size;
		save_sizeleft = size;
	      }

	    /* Locate data, determine max length writeable, write it,
	       record that we have used the data, then check if the write
	       worked.  */

	    data = findrec ()->charptr;
	    if (data == NULL)
	      {

		/* Check it.  */

		ERROR ((0, 0, _("Unexpected EOF on archive file")));
		break;
	      }

	    /* JK - If the file is sparse, use the sparsearray that we
	       created before to lseek into the new file the proper
	       amount, and to see how many bytes we want to write at that
	       position.  */

#if 0
	    if (head->header.linkflag == LF_SPARSE)
	      {
		off_t pos;

		pos = lseek (fd, (off_t) sparsearray[sparse_ind].offset, 0);
		fprintf (msg_file, _("%d at %d\n"), (int) pos, sparse_ind);
		written = sparsearray[sparse_ind++].numbytes;
	      }
	    else
#endif
	      written = endofrecs ()->charptr - data;

	    if (written > size)
	      written = size;
	    errno = 0;
	    check = write (fd, data, (size_t) written);

	    /* The following is in violation of strict typing, since the
	       arg to userec should be a struct rec *.  FIXME.  */

	    userec ((union record *) (data + written - 1));
	    if (check == written)
	      continue;

	    /* Error in writing to file.  Print it, skip to next file in
	       archive.  */

	    if (check < 0)
	      ERROR ((0, errno, _("Could not write to file %s"),
		      CURRENT_FILE_NAME));
	    else
	      ERROR ((0, 0, _("Could only write %d of %d bytes to file %s"),
		      check, written, CURRENT_FILE_NAME));
	    skip_file ((long) (size - written));
	    break;		/* still do the close, mod time, chmod, etc */
	  }

      if (flag_multivol)
	assign_string (&save_name, NULL);

      /* If writing to stdout, don't try to do anything to the filename;
	 it doesn't exist, or we don't want to touch it anyway.  */

      if (flag_exstdout)
	break;

#if 0
      if (head->header.isextended)
	{
	  register union record *exhdr;
	  register int i;

	  for (i = 0; i < 21; i++)
	    {
	      off_t offset;

	      if (!exhdr->ext_hdr.sp[i].numbytes)
		break;
	      offset = from_oct (1 + 12, exhdr->ext_hdr.sp[i].offset);
	      written = from_oct (1 + 12, exhdr->ext_hdr.sp[i].numbytes);
	      lseek (fd, offset, 0);
	      check = write (fd, data, written);
	      if (check == written)
		continue;
	    }
	}
#endif
      check = close (fd);
      if (check < 0)
	ERROR ((0, errno, _("Error while closing %s"), CURRENT_FILE_NAME));

    set_filestat:

      /* Set the modified time of the file.

	 Note that we set the accessed time to "now", which is really
	 "the time we started extracting files".  unless flag_gnudump is
	 used, in which case .st_atime is used  */

      if (!flag_modified)
	{

	  /* FIXME.  If flag_gnudump should set ctime too, but how?  */

	  if (flag_gnudump)
	    acc_upd_times.actime = hstat.st_atime;
	  else
	    acc_upd_times.actime = now;		/* accessed now */
	  acc_upd_times.modtime = hstat.st_mtime;	/* mod'd */
	  if (utime (CURRENT_FILE_NAME, &acc_upd_times) < 0)
	    ERROR ((0, errno,
		    _("Could not change access and modification times of %s"),
		    CURRENT_FILE_NAME));
	}

      /* We do the utime before the chmod because some versions of utime
	 are broken and trash the modes of the file.  Since we then
	 change the mode anyway, we don't care.  */

      /* If we are root, set the owner and group of the extracted file.
	 This does what is wanted both on real Unix and on System V.  If
	 we are running as a user, we extract as that user; if running as
	 root, we extract as the original owner.  */

      if (we_are_root || flag_do_chown)
	if (chown (CURRENT_FILE_NAME, hstat.st_uid, hstat.st_gid) < 0)
	  ERROR ((0, errno, _("Cannot chown file %s to uid %d gid %d"),
		  CURRENT_FILE_NAME, hstat.st_uid, hstat.st_gid));

      /* If '-k' is not set, open() or creat() could have saved the
	 permission bits from a previously created file, ignoring the
	 ones we specified.  Even if -k is set, if the file has abnormal
	 mode bits, we must chmod since writing or chown() has probably
	 reset them.

	 If -k is set, we know *we* created this file, so the mode bits
	 were set by our open().  If the file is "normal", we skip the
	 chmod.  This works because we did umask(0) if -p is set, so
	 umask will have left the specified mode alone.  */

      if ((!flag_keep)
	  || (hstat.st_mode & (S_ISUID | S_ISGID | S_ISVTX)))
	if (chmod (CURRENT_FILE_NAME, notumask & (int) hstat.st_mode) < 0)
	  ERROR ((0, errno, _("Cannot change mode of file %s to %lo"),
		  CURRENT_FILE_NAME, notumask & (int) hstat.st_mode));

    quit:
      break;

    case LF_LINK:
    again_link:
      {
	struct stat st1, st2;

	check = link (current_link_name, CURRENT_FILE_NAME);

	if (check == 0)
	  break;
	if (make_dirs (CURRENT_FILE_NAME))
	  goto again_link;
	if (flag_gnudump && errno == EEXIST)
	  break;
	if (stat (current_link_name, &st1) == 0
	    && stat (CURRENT_FILE_NAME, &st2) == 0
	    && st1.st_dev == st2.st_dev
	    && st1.st_ino == st2.st_ino)
	  break;
	ERROR ((0, errno, _("Could not link %s to %s"),
		CURRENT_FILE_NAME, current_link_name));
      }
      break;

#ifdef S_ISLNK
    case LF_SYMLINK:
    again_symlink:
      check = symlink (current_link_name, CURRENT_FILE_NAME);

      /* FIXME, don't worry uid, gid, etc.  */

      if (check == 0)
	break;
      if (make_dirs (CURRENT_FILE_NAME))
	goto again_symlink;

      ERROR ((0, errno, _("Could not create symlink to %s"),
	      current_link_name));
      break;
#endif

#ifdef S_IFCHR
    case LF_CHR:
      hstat.st_mode |= S_IFCHR;
      goto make_node;
#endif

#ifdef S_IFBLK
    case LF_BLK:
      hstat.st_mode |= S_IFBLK;
#endif
#if defined(S_IFCHR) || defined(S_IFBLK)
    make_node:
      check = mknod (CURRENT_FILE_NAME, (int) hstat.st_mode,
		     (int) hstat.st_rdev);
      if (check != 0)
	{
	  if (make_dirs (CURRENT_FILE_NAME))
	    goto make_node;
	  ERROR ((0, errno, _("Could not make %s"), CURRENT_FILE_NAME));
	  break;
	};
      goto set_filestat;
#endif

#ifdef S_ISFIFO
      /* If local system doesn't support FIFOs, use default case.  */

    case LF_FIFO:
    make_fifo:
      check = mkfifo (CURRENT_FILE_NAME, (int) hstat.st_mode);
      if (check != 0)
	{
	  if (make_dirs (CURRENT_FILE_NAME))
	    goto make_fifo;
	  ERROR ((0, errno, _("Could not make %s"), CURRENT_FILE_NAME));
	  break;
	};
      goto set_filestat;
#endif

    case LF_DIR:
    case LF_DUMPDIR:
      namelen = strlen (CURRENT_FILE_NAME) - 1;

    really_dir:
      /* Check for trailing /, and zap as many as we find.  */

      while (namelen && CURRENT_FILE_NAME[namelen] == '/')
	CURRENT_FILE_NAME[namelen--] = '\0';
      if (flag_gnudump)
	{

	  /* Read the entry and delete files that aren't listed in the
	     archive.  */

	  gnu_restore (skipcrud);
	}
      else if (head->header.linkflag == LF_DUMPDIR)
	skip_file ((long) (hstat.st_size));


    again_dir:
      check = mkdir (CURRENT_FILE_NAME,
		     (we_are_root ? 0 : 0300) | (int) hstat.st_mode);
      if (check != 0)
	{
	  struct stat st1;

	  if (make_dirs (CURRENT_FILE_NAME))
	    goto again_dir;

	  /* If we're trying to create '.', let it be.  */

	  if (CURRENT_FILE_NAME[namelen] == '.'
	      && (namelen == 0 || CURRENT_FILE_NAME[namelen - 1] == '/'))
	    goto check_perms;

	  if (errno == EEXIST && stat (CURRENT_FILE_NAME, &st1) == 0
	      && (S_ISDIR (st1.st_mode)))
	    break;

	  ERROR ((0, errno, _("Could not create directory %s"),
		  CURRENT_FILE_NAME));
	  break;
	}

    check_perms:
      if (!we_are_root && 0300 != (0300 & (int) hstat.st_mode))
	{
	  hstat.st_mode |= 0300;
	  WARN ((0, 0, _("Added write and execute permission to directory %s"),
		 CURRENT_FILE_NAME));
	}

      if (!flag_modified)
	{
	  tmp = ((struct saved_dir_info *)
		 tar_xmalloc (sizeof (struct saved_dir_info)));

	  tmp->path = xstrdup (CURRENT_FILE_NAME);
	  tmp->mode = hstat.st_mode;
	  tmp->atime = hstat.st_atime;
	  tmp->mtime = hstat.st_mtime;
	  tmp->uid = hstat.st_uid;
	  tmp->gid = hstat.st_gid;
	  tmp->next = saved_dir_info_head;
	  saved_dir_info_head = tmp;
	}
      else
	{
	  /* This functions exactly as the code for set_filestat above.  */

	  if (!flag_keep || (hstat.st_mode & (S_ISUID | S_ISGID | S_ISVTX)))
	    if (chmod (CURRENT_FILE_NAME, notumask & (int) hstat.st_mode) < 0)
	      ERROR ((0, errno, _("Cannot change mode of file %s to %lo"),
		      CURRENT_FILE_NAME, notumask & (int) hstat.st_mode));
	}
      break;

    case LF_VOLHDR:
      if (flag_verbose)
	fprintf (stdlis, _("Reading %s\n"), current_file_name);
      break;

    case LF_NAMES:
      extract_mangle ();
      break;

    case LF_MULTIVOL:
      ERROR ((0, 0, _("\
Cannot extract `%s' -- file is continued from another volume"),
	      current_file_name));
      skip_file ((long) hstat.st_size);
      break;

    case LF_LONGNAME:
    case LF_LONGLINK:
      ERROR ((0, 0, _("Visible long name error")));
      skip_file ((long) hstat.st_size);
      break;
    }

  /* We don't need to save it any longer.  */

  saverec (NULL);		/* unsave it */

#undef CURRENT_FILE_NAME
}

/*------------------------------------------------------------------------.
| After a file/link/symlink/dir creation has failed, see if it's because  |
| some required directory was not present, and if so, create all required |
| dirs.									  |
`------------------------------------------------------------------------*/

static int
make_dirs (char *pathname)
{
  char *p;			/* points into path */
  int madeone = 0;		/* did we do anything yet? */
  int save_errno = errno;	/* remember caller's errno */
  int check;

  if (errno != ENOENT)
    return 0;			/* not our problem */

  for (p = strchr (pathname, '/'); p != NULL; p = strchr (p + 1, '/'))
    {

      /* Avoid mkdir of empty string, if leading or double '/'.  */

      if (p == pathname || p[-1] == '/')
	continue;

      /* Avoid mkdir where last part of path is '.'.  */

      if (p[-1] == '.' && (p == pathname + 1 || p[-2] == '/'))
	continue;
      *p = 0;			/* truncate the path there */
      check = mkdir (pathname, 0777);	/* try to create it as a dir */
      if (check == 0)
	{

	  /* Fix ownership.  */

	  if (we_are_root)
	    if (chown (pathname, hstat.st_uid, hstat.st_gid) < 0)
	      ERROR ((0, errno,
		      _("Cannot change owner of %s to uid %d gid %d"),
		      pathname, hstat.st_uid, hstat.st_gid));

	  pr_mkdir (pathname, p - pathname, notumask & 0777);
	  madeone++;		/* remember if we made one */
	  *p = '/';
	  continue;
	}
      *p = '/';
      if (errno == EEXIST
#ifdef MSDOS
	  /* Turbo C mkdir gives a funny errno.  */
	  || errno == EACCES
#endif
	  )
	/* Directory already exists.  */
	continue;

      /* Some other error in the mkdir.  We return to the caller.  */

      break;
    }

  errno = save_errno;		/* restore caller's errno */
  return madeone;		/* tell them to retry if we made one */
}

/*---.
| ?  |
`---*/

static void
extract_sparse_file (int fd, long *sizeleft, long totalsize, char *name)
{
#if 0
  register char *data;
#endif
  union record *datarec;
  int sparse_ind = 0;
  int written, count;

  /* FIXME: `datarec' might be used uninitialized in this function.
     Reported by Bruno Haible.  */

  /* assuming sizeleft is initially totalsize */

  while (*sizeleft > 0)
    {
      datarec = findrec ();
      if (datarec == NULL)
	{
	  ERROR ((0, 0, _("Unexpected EOF on archive file")));
	  return;
	}
      lseek (fd, sparsearray[sparse_ind].offset, 0);
      written = sparsearray[sparse_ind++].numbytes;
      while (written > RECORDSIZE)
	{
	  count = write (fd, datarec->charptr, RECORDSIZE);
	  if (count < 0)
	    ERROR ((0, errno, _("Could not write to file %s"), name));
	  written -= count;
	  *sizeleft -= count;
	  userec (datarec);
	  datarec = findrec ();
	}

      count = write (fd, datarec->charptr, (size_t) written);

      if (count < 0)
	ERROR ((0, errno, _("Could not write to file %s"), name));
      else if (count != written)
	{
	  ERROR ((0, 0, _("Could only write %d of %d bytes to file %s"),
		     count, totalsize, name));
	  skip_file ((long) (*sizeleft));
	}

      written -= count;
      *sizeleft -= count;
      userec (datarec);
    }
  free (sparsearray);
#if 0
  if (end_nulls)
    {
      register int i;

      fprintf (stdlis, "%d\n", (int) end_nulls);
      for (i = 0; i < end_nulls; i++)
	write (fd, "\000", 1);
    }
#endif
  userec (datarec);
}

/*----------------------------------------------------------------.
| Set back the utime and mode for all the extracted directories.  |
`----------------------------------------------------------------*/

void
restore_saved_dir_info (void)
{
  struct utimbuf acc_upd_times;

  while (saved_dir_info_head != NULL)
    {

      /* FIXME, if flag_gnudump should set ctime too, but how?  */

      if (flag_gnudump)
	acc_upd_times.actime = saved_dir_info_head->atime;
      else
	acc_upd_times.actime = now;	/* accessed now */
      acc_upd_times.modtime = saved_dir_info_head->mtime;	/* mod'd */
      if (utime (saved_dir_info_head->path, &acc_upd_times) < 0)
	ERROR ((0, errno,
		_("Could not change access and modification times of %s"),
		saved_dir_info_head->path));

      /* If we are root, set the owner and group of the extracted file.
	 This does what is wanted both on real Unix and on System V.  If
	 we are running as a user, we extract as that user; if running as
	 root, we extract as the original owner.  */

      if (we_are_root || flag_do_chown)
	if (chown (saved_dir_info_head->path,
		   saved_dir_info_head->uid, saved_dir_info_head->gid) < 0)
	  ERROR ((0, errno, _("Cannot chown file %s to uid %d gid %d"),
		  saved_dir_info_head->path,
		  saved_dir_info_head->uid, saved_dir_info_head->gid));

      if ((!flag_keep)
	  || (saved_dir_info_head->mode & (S_ISUID | S_ISGID | S_ISVTX)))
	if (chmod (saved_dir_info_head->path,
		   notumask & saved_dir_info_head->mode)
	    < 0)
	  ERROR ((0, errno, _("Cannot change mode of file %s to %lo"),
		  saved_dir_info_head->path,
		  notumask & saved_dir_info_head->mode));

      saved_dir_info_head = saved_dir_info_head->next;
    }
}
