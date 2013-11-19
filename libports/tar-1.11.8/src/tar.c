/* Tar -- a tape archiver.
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

/* A tar (tape archiver) program.
   Written by John Gilmore, starting 25 Aug 85.  */

#include "system.h"

#ifndef FNM_LEADING_DIR
# include <fnmatch.h>
#endif

#ifndef S_ISLNK
# define lstat stat
#endif

#if WITH_REGEX
# include <regex.h>
#else
# include <rx.h>
#endif

/* The following causes "tar.h" to produce definitions of all the global
   variables, rather than just "extern" declarations of them.  */
#define GLOBAL
#include "tar.h"

/* We should use a conversion routine that does reasonable error checking
   -- atoi doesn't.  For now, punt.  FIXME.  */

#define intconv	atoi

time_t get_date ();

/* Local declarations.  */

#ifndef DEFAULT_ARCHIVE
# define DEFAULT_ARCHIVE "tar.out"
#endif

#ifndef DEFAULT_BLOCKING
# define DEFAULT_BLOCKING 20
#endif

time_t new_time;

/* Miscellaneous.  */

/*-------------------------------------------------------------------------.
| Assign STRING to a copy of VALUE if not NULL, or to NULL.  If STRING was |
| not NULL, it is freed first.						   |
`-------------------------------------------------------------------------*/

void
assign_string (char **string, const char *value)
{
  if (*string)
    free (*string);
  *string = value ? xstrdup (value) : NULL;
}

/*-----------------------------------------------------------------.
| Returns non-zero if the luser typed 'y' or 'Y', zero otherwise.  |
`-----------------------------------------------------------------*/

int
confirm (const char *action, const char *file)
{
  int c, nl;
  static FILE *confirm_file = 0;

  fprintf (stdlis, "%s %s?", action, file);
  fflush (stdlis);
  if (!confirm_file)
    {
      confirm_file = (archive == 0) ? fopen (TTY_NAME, "r") : stdin;
      if (!confirm_file)
	ERROR ((TAREXIT_FAILURE, 0, _("Cannot read confirmation from user")));
    }
  c = getc (confirm_file);
  for (nl = c; nl != '\n' && nl != EOF; nl = getc (confirm_file))
    ;
  return c == 'y' || c == 'Y';
}

/* Names from the command call.  */

static const char **name_array;	/* store an array of names */
static int allocated_names;	/* how big is the array? */
static int names;		/* how many entries does it have? */
static int name_index = 0;	/* how many of the entries have we scanned? */

/*--------------------------------------------------------------.
| Add NAME at end of name_array, reallocating it as necessary.  |
`--------------------------------------------------------------*/

static void
name_add (const char *name)
{
  if (names == allocated_names)
    {
      allocated_names *= 2;
      name_array = (const char **)
	tar_realloc (name_array, sizeof (const char *) * allocated_names);
    }
  name_array[names++] = name;
}

/* Names from external name file.  */

static FILE *name_file;		/* file to read names from */
static char *const *names_argv;	/* argv used by name routines */
static int names_argc;		/* argc used by name routines */

static char *name_buffer;	/* buffer to hold the current file name */
static size_t name_buffer_length; /* allocated length of name_buffer */

/*-------------------------------------------------------------------------.
| Set up to gather file names for tar.  They can either come from stdin or |
| from argv.								   |
`-------------------------------------------------------------------------*/

static void
name_init (int argc, char *const *argv)
{
  if (flag_namefile)
    {
      if (optind < argc)
	ERROR ((TAREXIT_FAILURE, 0, _("Too many args with -T option")));

      if (!strcmp (namefile_name, "-"))
	name_file = stdin;
      else if (name_file = fopen (namefile_name, "r"), !name_file)
	ERROR ((TAREXIT_FAILURE, errno, _("Cannot open file %s"), name_file));
    }
  else
    {

      /* Get file names from argv, after options. */

      names_argc = argc;
      names_argv = argv;
    }
}

/*---------------------------------------------------------------------.
| Read the next filename from name_file and null-terminate it.  Put it |
| into name_buffer, reallocating and adjusting name_buffer_length if   |
| necessary.  Return 0 at end of file, 1 otherwise.		       |
`---------------------------------------------------------------------*/

static int
read_name_from_file (void)
{
  register int c;
  register int counter = 0;

  /* FIXME: getc may be called even if c was EOF the last time here.  */

  /* FIXME: This + 2 allocation might serve no purpose.  */

  while (c = getc (name_file), c != EOF && c != filename_terminator)
    {
      if (counter == name_buffer_length)
	{
	  name_buffer_length += NAMSIZ;
	  name_buffer = tar_realloc (name_buffer, name_buffer_length + 2);
	}
      name_buffer[counter++] = c;
    }
  if (counter == 0 && c == EOF)
    return 0;
  if (counter == name_buffer_length)
    {
      name_buffer_length += NAMSIZ;
      name_buffer = tar_realloc (name_buffer, name_buffer_length + 2);
    }
  name_buffer[counter] = '\0';
  return 1;
}

/*-------------------------------------------------------------------------.
| Get the next name from argv or the name file.  Result is in static	   |
| storage and can't be relied upon across two calls.			   |
| 									   |
| If CHANGE_DIRS is non-zero, treat a filename of the form "-C" as meaning |
| that the next filename is the name of a directory to change to.  If	   |
| `filename_terminator' is '\0', CHANGE_DIRS is effectively always 0.	   |
`-------------------------------------------------------------------------*/

char *
name_next (int change_dirs)
{
  const char *source;
  char *cursor;
  int chdir_flag = 0;

  if (filename_terminator == '\0')
    change_dirs = 0;

  if (name_file)
    {

      /* Read from file.  */

      while (read_name_from_file ())
	if (*name_buffer)	/* ignore emtpy lines */
	  {

	    /* Zap trailing slashes.  */

	    cursor = name_buffer + strlen (name_buffer) - 1;
	    while (cursor > name_buffer && *cursor == '/')
	      *cursor-- = '\0';

	    if (chdir_flag)
	      {
		if (chdir (name_buffer) < 0)
		  ERROR ((TAREXIT_FAILURE, errno,
			  _("Cannot change to directory %s"), name_buffer));
		chdir_flag = 0;
	      }
	    else if (change_dirs && strcmp (name_buffer, "-C") == 0)
	      chdir_flag = 1;
	    else
#if 0
	      if (!flag_exclude || !check_exclude (name_buffer))
#endif
		return un_quote_string (name_buffer);
	  }

      /* No more names in file.  */

    }
  else
    {

      /* Read from argv, after options.  */

      while (1)
	{
	  if (name_index < names)
	    source = name_array[name_index++];
	  else if (optind < names_argc)
	    source = names_argv[optind++];
	  else
	    break;

	  if (strlen (source) > name_buffer_length)
	    {
	      free (name_buffer);
	      name_buffer_length = strlen (source);
	      name_buffer = tar_xmalloc (name_buffer_length + 2);
	    }
	  strcpy (name_buffer, source);

	  /* Zap trailing slashes.  */

	  cursor = name_buffer + strlen (name_buffer) - 1;
	  while (cursor > name_buffer && *cursor == '/')
	    *cursor-- = '\0';

	  if (chdir_flag)
	    {
	      if (chdir (name_buffer) < 0)
		ERROR ((TAREXIT_FAILURE, errno,
			_("Cannot chdir to %s"), name_buffer));
	      chdir_flag = 0;
	    }
	  else if (change_dirs && strcmp (name_buffer, "-C") == 0)
	    chdir_flag = 1;
	  else
#if 0
	    if (!flag_exclude || !check_exclude (name_buffer))
#endif
	      return un_quote_string (name_buffer);
	}
    }

  if (chdir_flag)
    ERROR ((TAREXIT_FAILURE, 0, _("Missing filename after -C")));
  return NULL;
}

/*------------------------------.
| Close the name file, if any.  |
`------------------------------*/

void
name_close (void)
{
  if (name_file != NULL && name_file != stdin)
    fclose (name_file);
}

/*-------------------------------------------------------------------------.
| Gather names in a list for scanning.  Could hash them later if we really |
| care.									   |
| 									   |
| If the names are already sorted to match the archive, we just read them  |
| one by one.  name_gather reads the first one, and it is called by	   |
| name_match as appropriate to read the next ones.  At EOF, the last name  |
| read is just left in the buffer.  This option lets users of small	   |
| machines extract an arbitrary number of files by doing "tar t" and	   |
| editing down the list of files.					   |
`-------------------------------------------------------------------------*/

void
name_gather (void)
{
  register char *p;
  static struct name *namebuf;	/* one-name buffer */
  static namelen;
  static char *chdir_name = NULL;

  if (flag_sorted_names)
    {
      if (!namelen)
	{
	  namelen = NAMSIZ;
	  namebuf = (struct name *) tar_xmalloc (sizeof (struct name) + NAMSIZ);
	}
      p = name_next (0);
      if (p)
	{
	  if (strcmp (p, "-C") == 0)
	    {
	      chdir_name = xstrdup (name_next (0));
	      p = name_next (0);
	      if (!p)
		ERROR ((TAREXIT_FAILURE, 0, _("Missing file name after -C")));
	      namebuf->change_dir = chdir_name;
	    }
	  namebuf->length = strlen (p);
	  if (namebuf->length >= namelen)
	    {
	      namebuf = (struct name *)
		tar_realloc (namebuf, sizeof (struct name) + namebuf->length);
	      namelen = namebuf->length;
	    }
	  strncpy (namebuf->name, p, (size_t) namebuf->length);
	  namebuf->name[namebuf->length] = 0;
	  namebuf->next = (struct name *) NULL;
	  namebuf->found = 0;
	  namelist = namebuf;
	  namelast = namelist;
	}
      return;
    }

  /* Non sorted names -- read them all in.  */

  while (p = name_next (0), p)
    addname (p);
}

/*-----------------------------.
| Add a name to the namelist.  |
`-----------------------------*/

void
addname (const char *name)
{
  register int i;		/* length of string */
  register struct name *p;	/* current struct pointer */
  static char *chdir_name = NULL;

  if (strcmp (name, "-C") == 0)
    {
      chdir_name = xstrdup (name_next (0));
      name = name_next (0);
      if (!chdir_name)
	ERROR ((TAREXIT_FAILURE, 0, _("Missing file name after -C")));
      if (chdir_name[0] != '/')
	{
	  char *path = tar_xmalloc (PATH_MAX);
#ifdef HAVE_GETCWD
	  if (!getcwd (path, PATH_MAX))
	    ERROR ((TAREXIT_FAILURE, 0, _("Could not get current directory")));
#else
	  char *getwd ();

	  if (!getwd (path))
	    ERROR ((TAREXIT_FAILURE, 0,
		    _("Could not get current directory: %s"), path));
#endif
	  chdir_name = xstrdup (new_name (path, chdir_name));
	  free (path);
	}
    }

  i = name ? strlen (name) : 0;
  p = (struct name *) tar_xmalloc ((unsigned) (sizeof (struct name) + i));
  p->next = (struct name *) NULL;
  if (name)
    {
      p->fake = 0;
      p->length = i;
      strncpy (p->name, name, (size_t) i);
      p->name[i] = '\0';	/* null term */
    }
  else
    p->fake = 1;
  p->found = 0;
  p->regexp = 0;		/* assume not a regular expression */
  p->firstch = 1;		/* assume first char is literal */
  p->change_dir = chdir_name;
  p->dir_contents = 0;		/* JF */
  if (name)
    {
      if (strchr (name, '*') || strchr (name, '[') || strchr (name, '?'))
	{
	  p->regexp = 1;	/* no, it's a regexp */
	  if (name[0] == '*' || name[0] == '[' || name[0] == '?')
	    p->firstch = 0;	/* not even 1st char literal */
	}
    }

  if (namelast)
    namelast->next = p;
  namelast = p;
  if (!namelist)
    namelist = p;
}

/*---------------------------------------------------------------------.
| Return nonzero if name P (from an archive) matches any name from the |
| namelist, zero if not.					       |
`---------------------------------------------------------------------*/

int
name_match (register const char *p)
{
  register struct name *nlp;
  register int len;

again:
  if (nlp = namelist, !nlp)	/* empty namelist is easy */
    return 1;
  if (nlp->fake)
    {
      if (nlp->change_dir && chdir (nlp->change_dir))
	ERROR ((TAREXIT_FAILURE, errno,
		_("Cannot change to directory %s"), nlp->change_dir));
      namelist = 0;
      return 1;
    }
  len = strlen (p);
  for (; nlp != 0; nlp = nlp->next)
    {

      /* If first chars don't match, quick skip.  */

      if (nlp->firstch && nlp->name[0] != p[0])
	continue;

      /* Regular expressions (shell globbing, actually).  */

      if (nlp->regexp)
	{
	  if (fnmatch (nlp->name, p, FNM_LEADING_DIR) == 0)
	    {
	      nlp->found = 1;	/* remember it matched */
	      if (flag_startfile)
		{
		  free ((void *) namelist);
		  namelist = 0;
		}
	      if (nlp->change_dir && chdir (nlp->change_dir))
		ERROR ((TAREXIT_FAILURE, errno,
			_("Cannot change to directory %s"), nlp->change_dir));

	      /* We got a match.  */
	      return 1;	
	    }
	  continue;
	}

      /* Plain Old Strings.  */

      if (nlp->length <= len	/* archive len >= specified */
	  && (p[nlp->length] == '\0' || p[nlp->length] == '/')
				/* full match on file/dirname */
	  && strncmp (p, nlp->name, (size_t) nlp->length) == 0)
				/* name compare */
	{
	  nlp->found = 1;	/* remember it matched */
	  if (flag_startfile)
	    {
	      free ((void *) namelist);
	      namelist = 0;
	    }
	  if (nlp->change_dir && chdir (nlp->change_dir))
	    ERROR ((TAREXIT_FAILURE, errno,
		    _("Cannot change to directory %s"), nlp->change_dir));

	  /* We got a match.  */
	  return 1;
	}
    }

  /* Filename from archive not found in namelist.  If we have the whole
     namelist here, just return 0.  Otherwise, read the next name in and
     compare it.  If this was the last name, namelist->found will remain
     on.  If not, we loop to compare the newly read name.  */

  if (flag_sorted_names && namelist->found)
    {
      name_gather ();		/* read one more */
      if (!namelist->found)
	goto again;
    }
  return 0;
}

/*------------------------------------------------------------------.
| Print the names of things in the namelist that were not matched.  |
`------------------------------------------------------------------*/

void
names_notfound (void)
{
  register struct name *nlp, *next;
  register char *p;

  for (nlp = namelist; nlp != 0; nlp = next)
    {
      next = nlp->next;
      if (!nlp->found)
	ERROR ((0, 0, _("%s: Not found in archive"), nlp->name));

      /* We could free() the list, but the process is about to die
	 anyway, so save some CPU time.  Amigas and other similarly
	 broken software will need to waste the time, though.  */

#ifdef amiga
      if (!flag_sorted_names)
	free (nlp);
#endif
    }
  namelist = (struct name *) NULL;
  namelast = (struct name *) NULL;

  if (flag_sorted_names)
    {
      while (p = name_next (1), p)
	ERROR ((0, 0, _("%s: Not found in archive"), p));
    }
}

/* These next routines were created by JF */

/*---.
| ?  |
`---*/

void
name_expand (void)
{
  ;
}

/*------------------------------------------------------------------------.
| This is like name_match(), except that it returns a pointer to the name |
| it matched, and doesn't set ->found The caller will have to do that if  |
| it wants to.  Oh, and if the namelist is empty, it returns 0, unlike	  |
| name_match(), which returns TRUE					  |
`------------------------------------------------------------------------*/

struct name *
name_scan (register const char *p)
{
  register struct name *nlp;
  register int len;

again:
  if (nlp = namelist, !nlp)	/* empty namelist is easy */
    return 0;
  len = strlen (p);
  for (; nlp != 0; nlp = nlp->next)
    {

      /* If first chars don't match, quick skip.  */

      if (nlp->firstch && nlp->name[0] != p[0])
	continue;

      /* Regular expressions.  */

      if (nlp->regexp)
	{
	  if (fnmatch (nlp->name, p, FNM_LEADING_DIR) == 0)
	    return nlp;		/* we got a match */
	  continue;
	}

      /* Plain Old Strings.  */

      if (nlp->length <= len	/* archive len >= specified */
	  && (p[nlp->length] == '\0' || p[nlp->length] == '/')
				/* full match on file/dirname */
	  && strncmp (p, nlp->name, (size_t) nlp->length) == 0)
				/* name compare */
	return nlp;		/* we got a match */
    }

  /* Filename from archive not found in namelist.  If we have the whole
     namelist here, just return 0.  Otherwise, read the next name in and
     compare it.  If this was the last name, namelist->found will remain
     on.  If not, we loop to compare the newly read name.  */

  if (flag_sorted_names && namelist->found)
    {
      name_gather ();		/* read one more */
      if (!namelist->found)
	goto again;
    }
  return NULL;
}

/*-----------------------------------------------------------------------.
| This returns a name from the namelist which doesn't have ->found set.	 |
| It sets ->found before returning, so successive calls will find and	 |
| return all the non-found names in the namelist			 |
`-----------------------------------------------------------------------*/

struct name *gnu_list_name;

char *
name_from_list (void)
{
  if (!gnu_list_name)
    gnu_list_name = namelist;
  while (gnu_list_name && gnu_list_name->found)
    gnu_list_name = gnu_list_name->next;
  if (gnu_list_name)
    {
      gnu_list_name->found++;
      if (gnu_list_name->change_dir)
	if (chdir (gnu_list_name->change_dir) < 0)
	  ERROR ((TAREXIT_FAILURE, errno, _("Cannot chdir to %s"),
		  gnu_list_name->change_dir));
      return gnu_list_name->name;
    }
  return NULL;
}

/*---.
| ?  |
`---*/

void
blank_name_list (void)
{
  struct name *n;

  gnu_list_name = 0;
  for (n = namelist; n; n = n->next)
    n->found = 0;
}

/*---.
| ?  |
`---*/

char *
new_name (char *path, char *name)
{
  char *path_buf;

  path_buf = (char *) tar_xmalloc (strlen (path) + strlen (name) + 2);
  sprintf (path_buf, "%s/%s", path, name);
  return path_buf;
}

/* Excludes.  */

char *x_buffer = NULL;
int size_x_buffer;
int free_x_buffer;

char **exclude = NULL;
int size_exclude = 0;
int free_exclude = 0;

char **re_exclude = NULL;
int size_re_exclude = 0;
int free_re_exclude = 0;

/*---.
| ?  |
`---*/

static int
is_regex (const char *str)
{
  return strchr (str, '*') || strchr (str, '[') || strchr (str, '?');
}

/*---.
| ?  |
`---*/

static void
add_exclude (char *name)
{
  int size_buf;

  un_quote_string (name);
  size_buf = strlen (name);

  if (x_buffer == 0)
    {
      x_buffer = (char *) tar_xmalloc ((size_t) (size_buf + 1024));
      free_x_buffer = 1024;
    }
  else if (free_x_buffer <= size_buf)
    {
      char *old_x_buffer;
      char **tmp_ptr;

      old_x_buffer = x_buffer;
      x_buffer = (char *) tar_realloc (x_buffer, (size_t) (size_x_buffer + 1024));
      free_x_buffer = 1024;
      for (tmp_ptr = exclude; tmp_ptr < exclude + size_exclude; tmp_ptr++)
	*tmp_ptr = x_buffer + ((*tmp_ptr) - old_x_buffer);
      for (tmp_ptr = re_exclude;
	   tmp_ptr < re_exclude + size_re_exclude;
	   tmp_ptr++)
	*tmp_ptr = x_buffer + ((*tmp_ptr) - old_x_buffer);
    }

  if (is_regex (name))
    {
      if (free_re_exclude == 0)
	{
	  re_exclude = (char **)
	    tar_realloc (re_exclude, (size_re_exclude + 32) * sizeof (char *));
	  free_re_exclude += 32;
	}
      re_exclude[size_re_exclude] = x_buffer + size_x_buffer;
      size_re_exclude++;
      free_re_exclude--;
    }
  else
    {
      if (free_exclude == 0)
	{
	  exclude = (char **)
	    tar_realloc (exclude, (size_exclude + 32) * sizeof (char *));
	  free_exclude += 32;
	}
      exclude[size_exclude] = x_buffer + size_x_buffer;
      size_exclude++;
      free_exclude--;
    }
  strcpy (x_buffer + size_x_buffer, name);
  size_x_buffer += size_buf + 1;
  free_x_buffer -= size_buf + 1;
}

/*---.
| ?  |
`---*/

static void
add_exclude_file (char *file)
{
  FILE *fp;
  char buf[1024];

  if (strcmp (file, "-"))
    fp = fopen (file, "r");
  else

    /* Let's hope the person knows what they're doing.  Using -X - -T -
       -f - will get you *really* strange results.  */

    fp = stdin;

  if (!fp)
    ERROR ((TAREXIT_FAILURE, errno, _("Cannot open %s"), file));

  while (fgets (buf, 1024, fp))
    {
#if 0
      int size_buf;
#endif
      char *end_str;

      end_str = strrchr (buf, '\n');
      if (end_str)
	*end_str = '\0';
      add_exclude (buf);

    }
  fclose (fp);
}

/*--------------------------------------------------------------------.
| Returns non-zero if the file 'name' should not be added/extracted.  |
`--------------------------------------------------------------------*/

int
check_exclude (const char *name)
{
  int n;
  char *str;

  for (n = 0; n < size_re_exclude; n++)
    {
      if (fnmatch (re_exclude[n], name, FNM_LEADING_DIR) == 0)
	return 1;
    }
  for (n = 0; n < size_exclude; n++)
    {

      /* Accept the output from strstr only if it is the last part of the
	 string.  There is certainly a faster way to do this.  */

      if (str = strstr (name, exclude[n]),
	  (str && (str == name || str[-1] == '/')
	   && str[strlen (exclude[n])] == '\0'))
	return 1;
    }
  return 0;
}


/* Options.  */

/* For long options that unconditionally set a single flag, we have getopt
   do it.  For the others, we share the code for the equivalent short
   named option, the name of which is stored in the otherwise-unused `val'
   field of the `struct option'; for long options that have no equivalent
   short option, we use nongraphic characters as pseudo short option
   characters, starting (for no particular reason) with character 10. */

#define OPTION_PRESERVE		10
#define OPTION_NEWER_MTIME	11
#define OPTION_DELETE		12
#define OPTION_EXCLUDE		13
#define OPTION_NULL		14
#define OPTION_VOLNO_FILE	15
#define OPTION_COMPRESS_PROG	16
#define OPTION_RSH_COMMAND	17

/* Some cleanup is made in GNU tar long options.  Using old names will send
   a warning to stderr.  */

#define OBSOLETE_ABSOLUTE_NAMES	20 /* take it out in 1.14 */

/* If non-zero, display usage information and exit.  */
static int show_help = 0;

/* If non-zero, print the version on standard output and exit.  */
static int show_version = 0;

struct option long_options[] =
{
  {"absolute-names", no_argument, NULL, 'P'},
  {"absolute-paths", no_argument, NULL, OBSOLETE_ABSOLUTE_NAMES},
  {"after-date", required_argument, NULL, 'N'},
  {"append", no_argument, NULL, 'r'},
  {"atime-preserve", no_argument, &flag_atime_preserve, 1},
  {"block-compress", no_argument, &flag_compress_block, 1},
  {"block-size", required_argument, NULL, 'b'},
  {"catenate", no_argument, NULL, 'A'},
  {"checkpoint", no_argument, &flag_checkpoint, 1},
  {"compare", no_argument, NULL, 'd'},
  {"compress", no_argument, NULL, 'Z'},
  {"concatenate", no_argument, NULL, 'A'},
  {"confirmation", no_argument, NULL, 'w'},
  {"create", no_argument, NULL, 'c'},
  {"delete", no_argument, NULL, OPTION_DELETE},
  {"dereference", no_argument, NULL, 'h'},
  {"diff", no_argument, NULL, 'd'},
  {"directory", required_argument, NULL, 'C'},
  {"exclude", required_argument, NULL, OPTION_EXCLUDE},
  {"exclude-from", required_argument, NULL, 'X'},
  {"extract", no_argument, NULL, 'x'},
  {"file", required_argument, NULL, 'f'},
  {"files-from", required_argument, NULL, 'T'},
  {"force-local", no_argument, &flag_force_local, 1},
  {"get", no_argument, NULL, 'x'},
  {"gunzip", no_argument, NULL, 'z'},
  {"gzip", no_argument, NULL, 'z'},
  {"help", no_argument, &show_help, 1},
  {"ignore-failed-read", no_argument, &flag_ignore_failed_read, 1},
  {"ignore-zeros", no_argument, NULL, 'i'},
  {"incremental", no_argument, NULL, 'G'},
  {"info-script", required_argument, NULL, 'F'},
  {"interactive", no_argument, NULL, 'w'},
  {"keep-old-files", no_argument, NULL, 'k'},
  {"label", required_argument, NULL, 'V'},
  {"list", no_argument, NULL, 't'},
  {"listed-incremental", required_argument, NULL, 'g'},
  {"modification-time", no_argument, NULL, 'm'},
  {"multi-volume", no_argument, NULL, 'M'},
  {"new-volume-script", required_argument, NULL, 'F'},
  {"newer", required_argument, NULL, 'N'},
  {"newer-mtime", required_argument, NULL, OPTION_NEWER_MTIME},
  {"null", no_argument, NULL, OPTION_NULL},
  {"old-archive", no_argument, NULL, 'o'},
  {"one-file-system", no_argument, NULL, 'l'},
  {"portability", no_argument, NULL, 'o'},
  {"preserve", no_argument, NULL, OPTION_PRESERVE},
  {"preserve-order", no_argument, NULL, 's'},
  {"preserve-permissions", no_argument, NULL, 'p'},
  {"read-full-blocks", no_argument, NULL, 'B'},
  {"record-number", no_argument, NULL, 'R'},
  {"remove-files", no_argument, &flag_remove_files, 1},
  {"rsh-command", required_argument, NULL, OPTION_RSH_COMMAND},
  {"same-order", no_argument, NULL, 's'},
  {"same-owner", no_argument, &flag_do_chown, 1},
  {"same-permissions", no_argument, NULL, 'p'},
  {"show-omitted-dirs", no_argument, &flag_show_omitted_dirs, 1},
  {"sparse", no_argument, NULL, 'S'},
  {"starting-file", required_argument, NULL, 'K'},
  {"tape-length", required_argument, NULL, 'L'},
  {"to-stdout", no_argument, NULL, 'O'},
  {"totals", no_argument, &flag_totals, 1},
  {"uncompress", no_argument, NULL, 'Z'},
  {"ungzip", no_argument, NULL, 'z'},
  {"update", no_argument, NULL, 'u'},
  {"use-compress-program", required_argument, NULL, OPTION_COMPRESS_PROG},
  {"verbose", no_argument, NULL, 'v'},
  {"verify", no_argument, NULL, 'W'},
  {"version", no_argument, &show_version, 1},
  {"volno-file", required_argument, NULL, OPTION_VOLNO_FILE},

  {0, 0, 0, 0}
};

/*---------------------------------------------.
| Print a usage message and exit with STATUS.  |
`---------------------------------------------*/

static void
usage (int status)
{
  if (status != TAREXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
      fputs (_("\
Mandatory or optional arguments to long options are mandatory or optional\n\
for short options too.\n"),
	     stdout);
      fputs(_("\
\n\
Main operation mode:\n\
  -t, --list              list the contents of an archive\n\
  -x, --extract, --get    extract files from an archive\n\
  -c, --create            create a new archive\n\
  -d, --diff, --compare   find differences between archive and file system\n\
  -r, --append            append files to the end of an archive\n\
  -u, --update            only append files newer than copy in archive\n\
  -A, --catenate          append tar files to an archive\n\
      --concatenate       same as -A\n\
      --delete            delete from the archive (not on mag tapes!)\n"),
	    stdout);
      fputs (_("\
\n\
Operation mode modificators:\n\
  -W, --verify               attempt to verify the archive after writing it\n\
      --remove-files         remove files after adding them to the archive\n\
  -k, --keep-old-files       don't overwrite existing files from archive\n\
  -S, --sparse               handle sparse files efficiently\n\
  -O, --to-stdout            extract files to standard output\n\
  -G, --incremental          handle old GNU-format incremental backup\n\
  -g, --listed-incremental   handle new GNU-format incremental backup\n\
      --ignore-failed-read   do not exit with non-zero on unreadable files\n"),
	     stdout);
      fputs (_("\
\n\
Handling of file attributes:\n\
      --atime-preserve         don't change access times on dumped files\n\
  -m, --modification-time      don't extract file modified time\n\
      --same-owner             create extracted files with the same ownership \n\
  -p, --same-permissions       extract all protection information\n\
      --preserve-permissions   same as -p\n\
  -s, --same-order             sort names to extract to match archive\n\
      --preserve-order         same as -s\n\
      --preserve               same as both -p and -s\n"),
	     stdout);
      fputs (_("\
\n\
Device selection and switching:\n\
  -f, --file=[HOSTNAME:]FILE     use archive file or device FILE on HOSTNAME\n\
      --force-local              archive file is local even if has a colon\n\
      --rsh-command=COMMAND      use remote COMMAND instead of rsh\n\
  -[0-7][lmh]                    specify drive and density\n\
  -M, --multi-volume             create/list/extract multi-volume archive\n\
  -L, --tape-length=NUM          change tape after writing NUM x 1024 bytes\n\
  -F, --info-script=FILE         run script at end of each tape (implies -M)\n\
      --new-volume-script=FILE   same as -F FILE\n"),
	     stdout);
      fputs (_("\
\n\
Device blocking:\n\
  -b, --block-size=BLOCKS    block size of BLOCKS x 512 bytes\n\
      --block-compress       block the output of compression for tapes\n\
  -i, --ignore-zeros         ignore blocks of zeros in archive (means EOF)\n\
  -B, --read-full-blocks     reblock as we read (for reading 4.2BSD pipes)\n"),
	     stdout);
      fputs (_("\
\n\
Archive format selection:\n\
  -V, --label=NAME                   create archive with volume name NAME\n\
  -o, --old-archive, --portability   write a V7 format archive (not ANSI)\n\
  -z, --gzip, --ungzip               filter the archive through gzip\n\
  -Z, --compress, --uncompress       filter the archive through compress\n\
      --use-compress-program=PROG    filter through PROG (must accept -d)\n"),
	     stdout);
      fputs (_("\
\n\
Local file selection:\n\
  -C, --directory DIR        change to directory DIR\n\
  -T, --files-from=NAME      get names to extract or create from file NAME\n\
      --null                 -T reads null-terminated names, disable -C\n\
      --exclude=FILE         exclude file FILE\n\
  -X, --exclude-from=FILE    exclude files listed in FILE\n\
  -P, --absolute-paths       don't strip leading `/'s from file names\n\
  -h, --dereference          dump instead the files symlinks point to\n\
  -l, --one-file-system      stay in local file system when creating archive\n\
  -K, --starting-file=NAME   begin at file NAME in the archive\n"),
	     stdout);
#ifndef MSDOS
      fputs (_("\
  -N, --newer=DATE           only store files newer than DATE\n\
      --after-date=DATE      same as -N\n"),
	     stdout);
#endif
      fputs (_("\
\n\
Informative output:\n\
      --help            print this help, then exit\n\
      --version         print tar program version number, then exit\n\
  -v, --verbose         verbosely list files processed\n\
      --checkpoint      print directory names while reading the archive\n\
      --totals          print total bytes written while creating archive\n\
  -R, --record-number   show record number within archive with each message\n\
  -w, --interactive     ask for confirmation for every action\n\
      --confirmation    same as -w\n"),
	     stdout);
      printf (_("\
\n\
On *this* particular tar, the defaults are -f %s and -b %d.\n"),
	      DEFAULT_ARCHIVE, DEFAULT_BLOCKING);
    }
  exit (status);
}

/*----------------------------.
| Parse the options for tar.  |
`----------------------------*/

#define OPTION_STRING \
  "-01234567ABC:F:GK:L:MN:OPRST:V:WX:Zb:cdf:g:hiklmoprstuvwxz"

#define SET_COMMAND_MODE(Mode) \
  (command_mode = command_mode == COMMAND_NONE ? (Mode) : COMMAND_TOO_MANY)

static void
decode_options (int argc, char *const *argv)
{
  int optchar;			/* option letter */

  /* Set some default option values.  */

  blocking = DEFAULT_BLOCKING;
  flag_rsh_command = NULL;

  /* Convert old-style tar call by exploding option element and rearranging
     options accordingly.  */

  if (argc > 1 && argv[1][0] != '-')
    {
      int new_argc;		/* argc value for rearranged arguments */
      char **new_argv;		/* argv value for rearranged arguments */
      char *const *in;		/* cursor into original argv */
      char **out;		/* cursor into rearranged argv */
      const char *letter;	/* cursor into old option letters */
      char buffer[3];		/* constructed option buffer */
      const char *cursor;	/* cursor in OPTION_STRING */

      /* Initialize a constructed option.  */

      buffer[0] = '-';
      buffer[2] = '\0';
      
      /* Allocate a new argument array, and copy program name in it.  */

      new_argc = argc - 1 + strlen (argv[1]);
      new_argv = (char **) tar_xmalloc (new_argc * sizeof (char *));
      in = argv;
      out = new_argv;
      *out++ = *in++;

      /* Copy each old letter option as a separate option, and have the
	 corresponding argument moved next to it.  */

      for (letter = *in++; *letter; letter++)
	{
	  buffer[1] = *letter;
	  *out++ = xstrdup (buffer);
	  cursor = strchr (OPTION_STRING, *letter);
	  if (cursor && cursor[1] == ':')
	    *out++ = *in++;
	}

      /* Copy all remaining options.  */

      while (in < argv + argc)
	*out++ = *in++;

      /* Replace the old option list by the new one.  */

      argc = new_argc;
      argv = new_argv;
    }

  /* Parse all options and non-options as they appear.  */

  while (optchar = getopt_long (argc, argv, OPTION_STRING, long_options, NULL),
	 optchar != EOF)
    switch (optchar)
      {
      case '?':
	usage (TAREXIT_FAILURE);

      case 0:
	break;

      case 'C':
	name_add ("-C");
	/* Fall through.  */

      case 1:
	/* File name or non-parsed option, because of RETURN_IN_ORDER
	   ordering triggerred by the leading dash in OPTION_STRING.  */

	name_add (optarg);
	break;

      case OPTION_PRESERVE:
	flag_use_protection = 1;
	flag_sorted_names = 1;
	break;

#ifndef MSDOS
      case OPTION_NEWER_MTIME:
	flag_new_files++;
	/* Fall through.  */
	/* FIXME: Something is wrong here!  */

      case 'N':
	/* Only write files newer than X.  */

	flag_new_files++;
	new_time = get_date (optarg, (voidstar) 0);
	if (new_time == (time_t) -1)
	  ERROR ((TAREXIT_FAILURE, 0, _("Invalid date format `%s'"),
		  optarg));
	break;
#endif /* not MSDOS */

      case OPTION_DELETE:
	SET_COMMAND_MODE (COMMAND_DELETE);
	break;

      case OPTION_EXCLUDE:
	flag_exclude++;
	add_exclude (optarg);
	break;

      case OPTION_NULL:
	filename_terminator = '\0';
	break;

      case OPTION_VOLNO_FILE:
	flag_volno_file = optarg;
	break;

      case OPTION_COMPRESS_PROG:
	if (flag_compressprog)
	  ERROR ((TAREXIT_FAILURE, 0,
		  _("Only one compression option permitted")));
	flag_compressprog = optarg;
	break;

      case OPTION_RSH_COMMAND:
	flag_rsh_command = optarg;
	break;

      case 'g':
	/* We are making a GNU dump; save directories at the beginning
	   of the archive, and include in each directory its contents.  */

	if (flag_oldarch)
	  usage (TAREXIT_FAILURE);
	flag_gnudump++;
	gnu_dumpfile = optarg;
	break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':

#ifdef DEVICE_PREFIX
	{
	  int device = optchar - '0';
	  int density;
	  static char buf[sizeof DEVICE_PREFIX + 10];
	  char *cursor;

	  density = getopt_long (argc, argv, "lmh", NULL, NULL);
	  strcpy (buf, DEVICE_PREFIX);
	  cursor = buf + strlen (buf);

#ifdef DENSITY_LETTER

	  sprintf (cursor, "%d%c", device, density);

#else /* not DENSITY_LETTER */

	  switch (density)
	    {
	    case 'l':
#ifdef LOW_NUM
	      device += LOW_NUM;
#endif
	      break;

	    case 'm':
#ifdef MID_NUM
	      device += MID_NUM;
#else
	      device += 8;
#endif
	      break;

	    case 'h':
#ifdef HGH_NUM
	      device += HGH_NUM;
#else
	      device += 16;
#endif
	      break;

	    default:
	      usage (TAREXIT_FAILURE);
	    }
	  sprintf (cursor, "%d", device);

#endif /* not DENSITY_LETTER */

	  if (archive_names == allocated_archive_names)
	    {
	      allocated_archive_names *= 2;
	      archive_name_array = (const char **)
		tar_realloc (archive_name_array,
			  sizeof (const char *) * allocated_archive_names);
	    }
	  archive_name_array[archive_names++] = buf;
	  /* FIXME: How comes this works for many archives when buf is
	     not xstrdup'ed?  */
	}
	break;

#else /* not DEVICE_PREFIX */

	ERROR ((0, 0, _("Options [0-7][lmh] not supported by *this* tar")));
	usage (TAREXIT_FAILURE);

#endif /* not DEVICE_PREFIX */

      case 'A':
	SET_COMMAND_MODE (COMMAND_CAT);
	break;

      case 'b':
	/* Set blocking factor.  */

	blocking = intconv (optarg);
	break;

      case 'B':
	/* Try to reblock input.  For reading 4.2BSD pipes.  */

	flag_reblock++;
	break;

      case 'c':
	SET_COMMAND_MODE (COMMAND_CREATE);
	break;

#if 0
      case 'C':
	if (chdir (optarg) < 0)
	  ERROR ((TAREXIT_FAILURE, errno,
		  _("Cannot change directory to %d"), optarg));
	break;
#endif

      case 'd':
	SET_COMMAND_MODE (COMMAND_DIFF);
	break;

      case 'f':
	if (archive_names == allocated_archive_names)
	  {
	    allocated_archive_names *= 2;
	    archive_name_array = (const char **)
	      tar_realloc (archive_name_array,
			sizeof (const char *) * allocated_archive_names);
	  }
	archive_name_array[archive_names++] = optarg;
	break;

      case 'F':
	/* Since -F is only useful with -M, make it implied.  Run this
	   script at the end of each tape.  */

	flag_run_script_at_end++;
	info_script = optarg;
	flag_multivol++;
	break;

      case 'G':
	/* We are making a GNU dump; save directories at the beginning
	   of the archive, and include in each directory its contents  */

	if (flag_oldarch)
	  usage (TAREXIT_FAILURE);
	flag_gnudump++;
	gnu_dumpfile = 0;
	break;

      case 'h':
	/* Follow symbolic links.  */

	flag_follow_links++;
	break;

      case 'i':
	/* Ignore zero records (eofs).  This can't be the default,
	   because Unix tar writes two records of zeros, then pads out
	   the block with garbage.  */

	flag_ignorez++;
	break;

      case 'k':
	/* Don't overwrite files.  */

#ifdef NO_OPEN3
	ERROR ((TAREXIT_FAILURE, 0,
		_("Cannot keep old files on this system")));
#else
	flag_keep++;
#endif
	break;

      case 'K':
	flag_startfile++;
	addname (optarg);
	break;

      case 'l':
	/* When dumping directories, don't dump files/subdirectories
	   that are on other filesystems.  */

	flag_local_filesys++;
	break;

      case 'L':
	tape_length = intconv (optarg);
	flag_multivol++;
	break;

      case 'm':
	flag_modified++;
	break;

      case 'M':
	/* Make multivolume archive: when we can't write any more into
	   the archive, re-open it, and continue writing.  */

	flag_multivol++;
	break;

      case 'o':
	/* Generate old archive.  */

	if (flag_gnudump
#if 0
	    || flag_dironly
#endif
	  )
	  usage (TAREXIT_FAILURE);
	flag_oldarch++;
	break;

      case 'O':
	flag_exstdout++;
	break;

      case 'p':
	flag_use_protection++;
	break;

      case OBSOLETE_ABSOLUTE_NAMES:
	WARN ((0, 0, _("Obsolete option name replaced by --absolute-names")));
	/* Fall through.  */

      case 'P':
	flag_absolute_names++;
	break;

      case 'r':
	SET_COMMAND_MODE (COMMAND_APPEND);
	break;

      case 'R':
	/* Print block numbers for debug of bad tar archives.  */

	flag_sayblock++;
	break;

      case 's':
	/* Names to extr are sorted.  */

	flag_sorted_names++;
	break;

      case 'S':
	/* Deal with sparse files.  */

	flag_sparse_files++;
	break;

      case 't':
	SET_COMMAND_MODE (COMMAND_LIST);

	/* "t" output == "cv" or "xv".  */

	flag_verbose++;
	break;

      case 'T':
	namefile_name = optarg;
	flag_namefile++;
	break;

      case 'u':
	SET_COMMAND_MODE (COMMAND_UPDATE);
	break;

      case 'v':
	flag_verbose++;
	break;

      case 'V':
	flag_volhdr = optarg;
	break;

      case 'w':
	flag_confirm++;
	break;

      case 'W':
	flag_verify++;
	break;

      case 'x':
	SET_COMMAND_MODE (COMMAND_EXTRACT);
	break;

      case 'X':
	flag_exclude++;
	add_exclude_file (optarg);
	break;

      case 'z':
	if (flag_compressprog)
	  ERROR ((TAREXIT_FAILURE, 0,
		  _("Only one compression option permitted")));
	flag_compressprog = "gzip";
	break;

      case 'Z':
	if (flag_compressprog)
	  ERROR ((TAREXIT_FAILURE, 0,
		  _("Only one compression option permitted")));
	flag_compressprog = "compress";
	break;
      }

  /* Process trivial options.  */

  if (show_version)
    {
      printf ("GNU %s %s\n", PACKAGE, VERSION);
      exit (TAREXIT_SUCCESS);
    }

  if (show_help)
    usage (TAREXIT_SUCCESS);

  /* Derive option values and check option consistency.  */

  blocksize = blocking * RECORDSIZE;

  if (archive_names == 0)
    {

      /* If no archive file name given, try TAPE from the environment, or
	 else, DEFAULT_ARCHIVE from the configuration process.  */

      archive_names = 1;
      archive_name_array[0] = getenv ("TAPE");
      if (archive_name_array[0] == NULL)
	archive_name_array[0] = DEFAULT_ARCHIVE;
    }

  archive_name_cursor = archive_name_array;
  if (archive_names > 1 && !flag_multivol)
    ERROR ((TAREXIT_FAILURE, 0,
	    _("Multiple archive files requires --multi-volume")));

  if (flag_compress_block && !flag_compressprog)
    ERROR ((TAREXIT_FAILURE, 0, _("\
You must use a compression option (--gzip, --compress\n\
or --use-compress-program) with --block-compress")));
}

#undef SET_COMMAND_MODE

/*-----------------------.
| Main routine for tar.	 |
`-----------------------*/

void
main (int argc, char **argv)
{
  program_name = argv[0];
  setlocale (LC_ALL, "");
  textdomain (PACKAGE);

  exit_status = TAREXIT_SUCCESS;
  filename_terminator = '\n';

  /* Pre-allocate a few structures.  */

  allocated_archive_names = 10;
  archive_name_array = (const char **)
    tar_xmalloc (sizeof (const char *) * allocated_archive_names);
  archive_names = 0;

  allocated_names = 10;
  name_array = (const char **)
    tar_xmalloc (sizeof (const char *) * allocated_names);
  names = 0;

  name_buffer = tar_xmalloc (NAMSIZ + 2);
  name_buffer_length = NAMSIZ;

  /* Decode options.  */

  decode_options (argc, argv);

  if (!names_argv)
    name_init (argc, argv);

  /* Main command execution.  */

  if (flag_volno_file)
    init_volume_number ();

  switch (command_mode)
    {

    case COMMAND_NONE:
      WARN ((0, 0, _("\
You must specify one of the r, c, t, x, or d options")));
      usage (TAREXIT_FAILURE);

    case COMMAND_TOO_MANY:
      WARN ((0, 0, _("\
You may not specify more than one of the r, c, t, x, or d options")));
      usage (TAREXIT_FAILURE);

    case COMMAND_CAT:
    case COMMAND_UPDATE:
    case COMMAND_APPEND:
      update_archive ();
      break;

    case COMMAND_DELETE:
      junk_archive ();
      break;

    case COMMAND_CREATE:
      create_archive ();
      if (flag_totals)
	fprintf (stderr, _("Total bytes written: %d\n"), tot_written);
      break;

    case COMMAND_EXTRACT:
      if (flag_volhdr)
	{
	  const char *err;

	  label_pattern = (struct re_pattern_buffer *)
	    tar_xmalloc (sizeof *label_pattern);
 	  memset (label_pattern, 0, sizeof *label_pattern);
	  err = re_compile_pattern (flag_volhdr, (int) strlen (flag_volhdr),
				    label_pattern);
	  if (err)
	    {
	      ERROR ((0, 0, _("Bad regular expression: %s"), err));
	      break;
	    }
	}
      extr_init ();
      read_and (extract_archive);
      break;

    case COMMAND_LIST:
      if (flag_volhdr)
	{
	  const char *err;

	  label_pattern = (struct re_pattern_buffer *)
	    tar_xmalloc (sizeof *label_pattern);
 	  memset (label_pattern, 0, sizeof *label_pattern);
	  err = re_compile_pattern (flag_volhdr, (int) strlen (flag_volhdr),
				    label_pattern);
	  if (err)
	    {
	      ERROR ((0, 0, _("Bad regular expression: %s"), err));
	      break;
	    }
	}
      read_and (list_archive);
#if 0
      if (!errors)
	errors = different;
#endif
      break;

    case COMMAND_DIFF:
      diff_init ();
      read_and (diff_archive);
      break;
    }

  if (flag_volno_file)
    closeout_volume_number ();

  /* Dispose of allocated memory, and return.  */

  free (name_buffer);
  free (name_array);
  free (archive_name_array);

  exit (exit_status);
}
