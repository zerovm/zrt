/* finddomain.c -- handle list of needed message catalogs
   Copyright (C) 1995 Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gettext.h"
#include "gettextP.h"
#include "libgettext.h"

/* @@ end of prolog @@ */

/* Contains the default location of the message catalogs.  */
const char _nl_default_dirname[] = DEF_MSG_DOM_DIR;

/* List with bindings of specific domains created by bindtextdomain()
   calls.  */
struct binding *_nl_domain_bindings;

/* List of already loaded domains.  */
extern struct loaded_domain *_nl_loaded_domains;

/* Prototypes for library functions.  */
char *xgetcwd ();
void *tar_xmalloc ();

/* Prototypes for local functions.  */
static const char *category_to_name __P((int category));
#if !defined HAVE_SETLOCALE || !defined HAVE_LC_MESSAGES
static const char *guess_category_value __P((int category,
					     const char *categoryname));
#endif


/* Return a data structure describing the message catalog described by
   the DOMAINNAME and CATEGORY parameters with respect to the currently
   established bindings.  */
struct loaded_domain *
_nl_find_domain (domainname, category)
     const char *domainname;
     int category;
{
  const char *categoryname;
  const char *categoryvalue;
  struct binding *binding;
  char *dirname;
  char *filename;
  size_t filename_len;
  struct loaded_domain *retval;

  /* Now determine the symbolic name of CATEGORY and its value.  */
  categoryname = category_to_name (category);
#ifdef HAVE_SETLOCALE
# ifndef HAVE_LC_MESSAGES
  if (category == LC_MESSAGES)
    categoryvalue = guess_category_value (category, categoryname);
  else
# endif
    categoryvalue = setlocale (category, NULL);
#else
  categoryvalue = guess_category_value (category, categoryname);
#endif

  /* If the current locale value is C (or POSIX) we don't load a domain.
     The causes the calling function to return the MSGID.  */ 
  if (strcmp (categoryvalue, "C") == 0 || strcmp (categoryvalue, "POSIX") == 0)
    return NULL;

  /* First find matching binding.  */
  for (binding = _nl_domain_bindings; binding != NULL; binding = binding->next)
    {
      int compare = strcmp (domainname, binding->domainname);
      if (compare == 0)
	/* We found it!  */
	break;
      if (compare < 0)
	{
	  /* It is not in the list.  */
	  binding = NULL;
	  break;
	}
    }

  if (binding == NULL)
    dirname = (char *) _nl_default_dirname;
  else if (binding->dirname[0] == '/')
    dirname = binding->dirname;
  else
    {
      /* We have a relative path.  Make it absolute now.  */
      char *cwd = xgetcwd ();

      dirname = alloca (strlen (cwd) + strlen (binding->dirname) + 1);
      stpcpy (stpcpy (dirname, cwd), binding->dirname);

      free (cwd);
    }

  /* Now we have all components which describe the message catalog to be
     used.  Create an absolute file name.  */
  filename_len = strlen (dirname) + 1 + strlen (categoryvalue) + 1
		 + strlen (categoryname) + 1 + strlen (domainname) + 4;
  filename = (char *) tar_xmalloc (filename_len);
  sprintf (filename, "%s/%s/%s/%s.mo", dirname, categoryvalue,
	   categoryname, domainname);

  /* Look in list of already loaded file whether it is already available.  */
  for (retval = _nl_loaded_domains; retval != NULL; retval = retval->next)
    {
      int compare = strcmp (retval->filename, filename);
      if (compare == 0)
	/* We found it!  */
	break;
      if (compare < 0)
	{
	  /* It's not in the list.  */
	  retval = NULL;
	  break;
	}
    }

  /* If it is not in list try to load it.  Free the allocated FILENAME
     if it is not used.  */
  if (retval != NULL || (retval = _nl_load_msg_cat (filename)) == NULL)
    free (filename);

  return retval;
}

/* Return string representation of locale CATEGORY.  */
static const char *category_to_name (category)
     int category;
{
  const char *retval;

  switch (category)
  {
#ifdef LC_COLLATE
  case LC_COLLATE:
    retval = "LC_COLLATE";
    break;
#endif
#ifdef LC_CTYPE
  case LC_CTYPE:
    retval = "LC_CTYPE";
    break;
#endif
#ifdef LC_MONETARY
  case LC_MONETARY:
    retval = "LC_MONETARY";
    break;
#endif
#ifdef LC_NUMERIC
  case LC_NUMERIC:
    retval = "LC_NUMERIC";
    break;
#endif
#ifdef LC_TIME
  case LC_TIME:
    retval = "LC_TIME";
    break;
#endif
#ifdef LC_MESSAGES
  case LC_MESSAGES:
    retval = "LC_MESSAGES";
    break;
#endif
#ifdef LC_RESPONSE
  case LC_RESPONSE:
    retval = "LC_RESPONSE";
    break;
#endif
#ifdef LC_ALL
  case LC_ALL:
    /* This might not make sense but is perhaps better than any other
       value.  */
    retval = "LC_ALL";
    break;
#endif
  default:
    /* If you have a better idea for a default value let me know.  */
    retval = "LC_XXX";
  }

  return retval;
}

#if !defined HAVE_SETLOCALE || !defined HAVE_LC_MESSAGES
/* Guess value of current locale from value of the environment variables.  */
static const char *guess_category_value (category, categoryname)
     int category;
     const char *categoryname;
{
  const char *retval;

  /* Setting of LC_ALL overwrites all other.  */
  retval = getenv ("LC_ALL");
  if (retval != NULL && retval[0] != '\0')
    return retval;

  /* Next comes the name of the desired category.  */
  retval = getenv (categoryname);
  if (retval != NULL && retval[0] != '\0')
    return retval;

  /* Last possibility is the LANG environment variable.  */
  retval = getenv ("LANG");
  if (retval != NULL && retval[0] != '\0')
    return retval;

  /* We use C as the default domain.  POSIX says this is implementation
     defined.  */
  return "C";
}
#endif
