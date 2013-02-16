
/* GNU dump extensions to tar.
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

#include "system.h"

#include <time.h>
time_t time ();

#define ISDIGIT(Char) (ISASCII (Char) && isdigit (Char))
#define ISSPACE(Char) (ISASCII (Char) && isspace (Char))

#ifndef S_ISLNK
#define lstat stat
#endif

#include "tar.h"

extern time_t new_time;

static void add_dir __P ((char *, dev_t, ino_t, const char *));
static void add_dir_name __P ((char *, int));
static int dirent_cmp __P ((const voidstar, const voidstar));
static int name_cmp __P ((struct name *, struct name *));
static int recursively_delete __P ((char *));

struct dirname
  {
    struct dirname *next;
    const char *name;
    const char *dir_text;
    int dev;
    int ino;
    int allnew;
  };
static struct dirname *dir_list;
static time_t this_time;

/*---.
| ?  |
`---*/

static void
add_dir (char *name, dev_t dev, ino_t ino, const char *text)
{
  struct dirname *dp;

  dp = (struct dirname *) xmalloc (sizeof (struct dirname));
  dp->next = dir_list;
  dir_list = dp;

  dp->dev = dev;
  dp->ino = ino;
  dp->name = xstrdup (name);
  dp->dir_text = text;
  dp->allnew = 0;
}

/*---.
| ?  |
`---*/

static void
read_dir_file (void)
{
  dev_t dev;
  ino_t ino;
  char *strp;
  FILE *fp;
  char buf[512];
  static char *path = 0;

  if (path == 0)
    path = xmalloc (PATH_MAX);
  time (&this_time);
  if (gnu_dumpfile[0] != '/')
    {
#ifdef HAVE_GETCWD
      if (!getcwd (path, PATH_MAX))
	ERROR ((TAREXIT_FAILURE, 0, _("Could not get current directory")));
#else
      char *getwd ();

      if (!getwd (path))
	ERROR ((TAREXIT_FAILURE, 0, _("Could not get current directory: %s"),
		path));
#endif

      /* If this doesn't fit, we're in serious trouble.  */

      strcat (path, "/");
      strcat (path, gnu_dumpfile);
      gnu_dumpfile = path;
    }
  fp = fopen (gnu_dumpfile, "r");
  if (fp == 0 && errno != ENOENT)
    {
      ERROR ((0, errno, _("Cannot open %s"), gnu_dumpfile));
      return;
    }
  if (!fp)
    return;
  fgets (buf, sizeof (buf), fp);
  if (!flag_new_files)
    {
      flag_new_files++;
      new_time = atol (buf);
    }
  while (fgets (buf, sizeof (buf), fp))
    {
      strp = &buf[strlen (buf)];
      if (strp[-1] == '\n')
	strp[-1] = '\0';
      strp = buf;
      dev = atol (strp);
      while (ISDIGIT (*strp))
	strp++;
      ino = atol (strp);
      while (ISSPACE (*strp))
	strp++;
      while (ISDIGIT (*strp))
	strp++;
      strp++;
      add_dir (un_quote_string (strp), dev, ino, (char *) 0);
    }
  fclose (fp);
}

/*---.
| ?  |
`---*/

void
write_dir_file (void)
{
  FILE *fp;
  struct dirname *dp;
  char *str;

  fp = fopen (gnu_dumpfile, "w");
  if (fp == 0)
    {
      ERROR ((0, errno, _("Cannot write to %s"), gnu_dumpfile));
      return;
    }
  fprintf (fp, "%lu\n", (long unsigned int)this_time);
  for (dp = dir_list; dp; dp = dp->next)
    {
      if (!dp->dir_text)
	continue;
      str = quote_copy_string (dp->name);
      if (str)
	{
	  fprintf (fp, "%u %u %s\n", dp->dev, dp->ino, str);
	  free (str);
	}
      else
	fprintf (fp, "%u %u %s\n", dp->dev, dp->ino, dp->name);
    }
  fclose (fp);
}

/*---.
| ?  |
`---*/

static struct dirname *
get_dir (char *name)
{
  struct dirname *dp;

  for (dp = dir_list; dp; dp = dp->next)
    {
      if (!strcmp (dp->name, name))
	return dp;
    }
  return 0;
}

/*-------------------------------------------------------------------------.
| Collect all the names from argv[] (or whatever), then expand them into a |
| directory tree, and put all the directories at the beginning.		   |
`-------------------------------------------------------------------------*/

void
collect_and_sort_names (void)
{
  struct name *n, *n_next;
  int num_names;
  struct stat statbuf;

  name_gather ();

  if (gnu_dumpfile)
    read_dir_file ();
  if (!namelist)
    addname (".");
  for (n = namelist; n; n = n_next)
    {
      n_next = n->next;
      if (n->found || n->dir_contents)
	continue;
      if (n->regexp)		/* FIXME just skip regexps for now */
	continue;
      if (n->change_dir)
	if (chdir (n->change_dir) < 0)
	  {
	    ERROR ((0, errno, _("Cannot chdir to %s"), n->change_dir));
	    continue;
	  }

      if (
#ifdef AIX
	  statx (n->name, &statbuf, STATSIZE, STX_HIDDEN | STX_LINK)
#else
	  lstat (n->name, &statbuf) < 0
#endif
	  )
	{
	  ERROR ((0, errno, _("Cannot stat %s"), n->name));
	  continue;
	}
      if (S_ISDIR (statbuf.st_mode))
	{
	  n->found++;
	  add_dir_name (n->name, statbuf.st_dev);
	}
    }

  num_names = 0;
  for (n = namelist; n; n = n->next)
    num_names++;
  namelist = (struct name *)
    merge_sort ((voidstar) namelist, (unsigned) num_names,
		(char *) (&(namelist->next)) - (char *) namelist, name_cmp);

  for (n = namelist; n; n = n->next)
    {
      n->found = 0;
    }
  if (gnu_dumpfile)
    write_dir_file ();
}

/*---.
| ?  |
`---*/

static int
name_cmp (struct name *n1, struct name *n2)
{
  if (n1->found)
    {
      if (n2->found)
	return strcmp (n1->name, n2->name);
      else
	return -1;
    }
  else if (n2->found)
    return 1;
  else
    return strcmp (n1->name, n2->name);
}

/*---.
| ?  |
`---*/

static int
dirent_cmp (const voidstar p1, const voidstar p2)
{
  char const *frst, *scnd;

  frst = (*(char *const *) p1) + 1;
  scnd = (*(char *const *) p2) + 1;

  return strcmp (frst, scnd);
}

/*---.
| ?  |
`---*/

char *
get_dir_contents (char *p, int device)
{
  DIR *dirp;
  register struct dirent *d;
  char *new_buf;
  char *namebuf;
  int bufsiz;
  int len;
  voidstar the_buffer;
  char *buf;
  size_t n_strs;
#if 0
  int n_size;
#endif
  char *p_buf;
  char **vec, **p_vec;

  errno = 0;
  dirp = opendir (p);
  bufsiz = strlen (p) + NAMSIZ;
  namebuf = xmalloc ((size_t) (bufsiz + 2));
  if (!dirp)
    {
      ERROR ((0, errno, _("Cannot open directory %s"), p));
      new_buf = NULL;
    }
  else
    {
      struct dirname *dp;
      int all_children;

      dp = get_dir (p);
      all_children = dp ? dp->allnew : 0;
      strcpy (namebuf, p);
      if (p[strlen (p) - 1] != '/')
	strcat (namebuf, "/");
      len = strlen (namebuf);

      the_buffer = init_buffer ();
      while (d = readdir (dirp), d)
	{
	  struct stat hs;

	  /* Skip `.' and `..'.  */

	  if (is_dot_or_dotdot (d->d_name))
	    continue;
	  if ((int) NAMLEN (d) + len >= bufsiz)
	    {
	      bufsiz += NAMSIZ;
	      namebuf = (char *) xrealloc (namebuf, (size_t) (bufsiz + 2));
	    }
	  strcpy (namebuf + len, d->d_name);
#ifdef AIX
	  if (flag_follow_links ? statx (namebuf, &hs, STATSIZE, STX_HIDDEN)
	      : statx (namebuf, &hs, STATSIZE, STX_HIDDEN | STX_LINK))
#else
	  if (flag_follow_links ? stat (namebuf, &hs) : lstat (namebuf, &hs))
#endif
	    {
	      ERROR ((0, errno, _("Cannot stat %s"), namebuf));
	      continue;
	    }
	  if ((flag_local_filesys && device != hs.st_dev)
	      || (flag_exclude && check_exclude (namebuf)))
	    add_buffer (the_buffer, "N", 1);
#ifdef AIX
	  else if (S_ISHIDDEN (hs.st_mode))
	    {
	      add_buffer (the_buffer, "D", 1);
	      strcat (d->d_name, "A");
	      d->d_namlen++;
	    }
#endif /* AIX */
	  else if (S_ISDIR (hs.st_mode))
	    {
	      if (dp = get_dir (namebuf), dp)
		{
		  if (dp->dev != hs.st_dev
		      || dp->ino != hs.st_ino)
		    {
		      if (flag_verbose)
			WARN ((0, 0, _("Directory %s has been renamed"),
			       namebuf));
		      dp->allnew = 1;
		      dp->dev = hs.st_dev;
		      dp->ino = hs.st_ino;
		    }
		  dp->dir_text = "";
		}
	      else
		{
		  if (flag_verbose)
		    WARN ((0, 0, _("Directory %s is new"), namebuf));
		  add_dir (namebuf, hs.st_dev, hs.st_ino, "");
		  dp = get_dir (namebuf);
		  dp->allnew = 1;
		}
	      if (all_children && dp)
		dp->allnew = 1;

	      add_buffer (the_buffer, "D", 1);
	    }
	  else if (!all_children
		   && flag_new_files
		   && new_time > hs.st_mtime
		   && (flag_new_files > 1
		       || new_time > hs.st_ctime))
	    add_buffer (the_buffer, "N", 1);
	  else
	    add_buffer (the_buffer, "Y", 1);
	  add_buffer (the_buffer, d->d_name, (int) (NAMLEN (d) + 1));
	}
      add_buffer (the_buffer, "\000\000", 2);
      closedir (dirp);

      /* Well, we've read in the contents of the dir, now sort them.  */

      buf = get_buffer (the_buffer);
      if (buf[0] == '\0')
	{
	  flush_buffer (the_buffer);
	  new_buf = NULL;
	}
      else
	{
	  n_strs = 0;
	  for (p_buf = buf; *p_buf;)
	    {
	      int tmp;

	      tmp = strlen (p_buf) + 1;
	      n_strs++;
	      p_buf += tmp;
	    }
	  vec = (char **) xmalloc (sizeof (char *) * (n_strs + 1));
	  for (p_vec = vec, p_buf = buf; *p_buf; p_buf += strlen (p_buf) + 1)
	    *p_vec++ = p_buf;
	  *p_vec = 0;
	  qsort ((voidstar) vec, n_strs, sizeof (char *), dirent_cmp);
	  new_buf = (char *) xmalloc ((size_t) (p_buf - buf + 2));
	  for (p_vec = vec, p_buf = new_buf; *p_vec; p_vec++)
	    {
	      char *p_tmp;

	      for (p_tmp = *p_vec; (*p_buf++ = *p_tmp++); )
		;
	    }
	  *p_buf++ = '\0';
	  free (vec);
	  flush_buffer (the_buffer);
	}
    }
  free (namebuf);
  return new_buf;
}

/*----------------------------------------------------------------------.
| P is a directory.  Add all the files in P to the namelist.  If any of |
| the files is a directory, recurse on the subdirectory.	        |
`----------------------------------------------------------------------*/

static void
add_dir_name (char *p, int device)
{
  char *new_buf;
  char *p_buf;

  char *namebuf;
  int buflen;
  register int len;
  int sublen;

#if 0
  voidstar the_buffer;

  char *buf;
  char **vec,**p_vec;
  int n_strs,n_size;
#endif

  struct name *n;

  new_buf = get_dir_contents (p, device);

  for (n = namelist; n; n = n->next)
    {
      if (!strcmp (n->name, p))
	{
	  n->dir_contents = new_buf ? new_buf : "\0\0\0\0";
	  break;
	}
    }

  if (new_buf)
    {
      len = strlen (p);
      buflen = NAMSIZ <= len ? len + NAMSIZ : NAMSIZ;
      namebuf = xmalloc ((size_t) (buflen + 1));

      strcpy (namebuf, p);
      if (namebuf[len - 1] != '/')
	{
	  namebuf[len++] = '/';
	  namebuf[len] = '\0';
	}
      for (p_buf = new_buf; p_buf && *p_buf; p_buf += sublen + 1)
	{
	  sublen = strlen (p_buf);
	  if (*p_buf == 'D')
	    {
	      if (len + sublen >= buflen)
		{
		  buflen += NAMSIZ;
		  namebuf = (char *) xrealloc (namebuf, (size_t) (buflen + 1));
		}
	      strcpy (namebuf + len, p_buf + 1);
	      addname (namebuf);
	      add_dir_name (namebuf, device);
	    }
	}
      free (namebuf);
    }
}

/*-------------------------------------------------------------------------.
| Returns non-zero if p is `.' or `..'.  This could be a macro for speed.  |
`-------------------------------------------------------------------------*/

/* Early Solaris 2.4 readdir may return d->d_name as `' in NFS-mounted
   directories.  The workaround here skips `' just like `.'.  Without it,
   GNU tar would then treat `' much like `.' and loop endlessly.  */

int
is_dot_or_dotdot (char *p)
{
  return (p[0] == '\0'
	  || (p[0] == '.' && (p[1] == '\0'
			      || (p[1] == '.' && p[2] == '\0'))));
}

/*---.
| ?  |
`---*/

void
gnu_restore (int skipcrud)
{
  char *current_dir;
#if 0
  int current_dir_length;
#endif
  char *archive_dir;
#if 0
  int archive_dir_length;
#endif
  voidstar the_buffer;
  char *p;
  DIR *dirp;
  struct dirent *d;
  char *cur, *arc;
  long size, copied;
  char *from, *to;

#define CURRENT_FILE_NAME (skipcrud + current_file_name)

  dirp = opendir (CURRENT_FILE_NAME);

  if (!dirp)
    {

      /* The directory doesn't exist now.  It'll be created.  In any
	 case, we don't have to delete any files out of it.  */

      skip_file ((long) hstat.st_size);
      return;
    }

  the_buffer = init_buffer ();
  while (d = readdir (dirp), d)
    {
      if (is_dot_or_dotdot (d->d_name))
	continue;

      add_buffer (the_buffer, d->d_name, (int) (NAMLEN (d) + 1));
    }
  closedir (dirp);
  add_buffer (the_buffer, "", 1);

  current_dir = get_buffer (the_buffer);
  archive_dir = (char *) ck_malloc ((size_t) hstat.st_size);
  if (archive_dir == 0)
    {
      ERROR ((0, 0, _("Cannot allocate %d bytes for restore"), hstat.st_size));
      skip_file ((long) hstat.st_size);
      return;
    }
  to = archive_dir;
  for (size = hstat.st_size; size > 0; size -= copied)
    {
      from = findrec ()->charptr;
      if (!from)
	{
	  ERROR ((0, 0, _("Unexpected EOF in archive")));
	  break;
	}
      copied = endofrecs ()->charptr - from;
      if (copied > size)
	copied = size;
      memcpy ((voidstar) to, (voidstar) from, (size_t) copied);
      to += copied;
      userec ((union record *) (from + copied - 1));
    }

  for (cur = current_dir; *cur; cur += strlen (cur) + 1)
    {
      for (arc = archive_dir; *arc; arc += strlen (arc) + 1)
	{
	  arc++;
	  if (!strcmp (arc, cur))
	    break;
	}
      if (*arc == '\0')
	{
	  p = new_name (CURRENT_FILE_NAME, cur);
	  if (flag_confirm && !confirm ("delete", p))
	    {
	      free (p);
	      continue;
	    }
	  if (flag_verbose)
	    fprintf (stdlis, _("%s: Deleting %s\n"), program_name, p);
	  if (recursively_delete (p))
	    ERROR ((0, 0, _("Error while deleting %s"), p));
	  free (p);
	}

    }
  flush_buffer (the_buffer);
  free (archive_dir);

#undef CURRENT_FILE_NAME
}

/*---.
| ?  |
`---*/

static int
recursively_delete (char *path)
{
  struct stat sbuf;
  DIR *dirp;
  struct dirent *dp;
  char *path_buf;
#if 0
  int path_len;
#endif

  if (lstat (path, &sbuf) < 0)
    return 1;
  if (S_ISDIR (sbuf.st_mode))
    {
#if 0
      path_len = strlen (path);
#endif
      dirp = opendir (path);
      if (dirp == 0)
	return 1;
      while (dp = readdir (dirp), dp)
	{
	  if (is_dot_or_dotdot (dp->d_name))
	    continue;
	  path_buf = new_name (path, dp->d_name);
	  if (recursively_delete (path_buf))
	    {
	      free (path_buf);
	      closedir (dirp);
	      return 1;
	    }
	  free (path_buf);
	}
      closedir (dirp);

      if (rmdir (path) < 0)
	return 1;
      return 0;
    }
  if (unlink (path) < 0)
    return 1;
  return 0;
}
