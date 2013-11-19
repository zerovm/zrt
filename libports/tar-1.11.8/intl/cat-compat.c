/* Compatibility code for gettext-using-catgets interface.
   Copyright (C) 1995 Free Software Foundation, Inc.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#ifdef HAVE_NL_TYPES_H
# include <nl_types.h>
#endif

#include "libgettext.h"

/* @@ end of prolog @@ */

extern char *xstrdup __P ((const char *));

/* The catalog descriptor.  */
static nl_catd catalog = (nl_catd) -1;

/* Name of the default catalog.  */
static char default_catalog_name[] = "messages";

/* Name of currently used catalog.  */
static char *catalog_name = default_catalog_name;

/* Get ID for given string.  If not found return -1.  */
static int msg_to_cat_id (const char *msg);


/* Set currently used domain/catalog.  */
char *
textdomain (domainname)
     const char *domainname;
{
  nl_catd new_catalog;
  char *new_name;
  size_t new_name_len;
  char *lang;

#if defined HAVE_SETLOCALE && defined HAVE_LC_MESSAGES
  lang = setlocale (LC_MESSAGES, NULL);
#else
  lang = getenv ("LC_MESSAGES");
  if (lang == NULL || lang[0] == '\0')
    lang = getenv ("LANG");
#endif
  if (lang == NULL || lang[0] == '\0')
    lang = "C";

  /* See whether name of currently used domain is asked.  */
  if (domainname == NULL)
    return catalog_name;

  if (domainname[0] == '\0')
    domainname = default_catalog_name;

  /* Compute length of added path element.  */
  new_name_len = sizeof (DEF_MSG_DOM_DIR) - 1 + 1 + strlen (lang)
		 + sizeof ("/LC_MESSAGES/") - 1 + sizeof (PACKAGE) - 1
		 + sizeof (".cat");

  new_name = (char *) tar_xmalloc (new_name_len);

  sprintf (new_name, "%s/%s/LC_MESSAGES/%s.cat", DEF_MSG_DOM_DIR, lang,
	   PACKAGE);

  new_catalog = catopen (new_name, 0);
  if (new_catalog == (nl_catd) -1)
    {
      /* The system seems not to understand an absolute file name as
	 argument to catopen.  Try now with the established NLSPATH.  */
      stpcpy (new_name, PACKAGE);

      new_catalog = catopen (new_name, 0);
      if (new_catalog == (nl_catd) -1)
	{
	  free (new_name);
	  return catalog_name;
	}
    }

  /* Close old catalog.  */
  if (catalog != (nl_catd) -1)
    catclose (catalog);
  if (catalog_name != default_catalog_name)
    free (catalog_name);

  catalog = new_catalog;
  catalog_name = new_name;

  return catalog_name;
}

char *
bindtextdomain (domainname, dirname)
     const char *domainname;
     const char *dirname;
{
#if defined HAVE_SETENV || defined HAVE_PUTENV
  char *old_val = getenv ("NLSPATH");
  char *new_val;
  size_t new_val_len;

  /* This does not make much sense here but to be compatible do it.  */
  if (domainname == NULL)
    return NULL;

  /* Compute length of added path element.  If we use setenv we don't need
     the first byts for NLSPATH=, but why complicate the code for this
     peanuts.  */
  new_val_len = sizeof ("NLSPATH=") - 1 + sizeof (DEF_MSG_DOM_DIR) - 1
		+ sizeof ("/%L/LC_MESSAGES/") - 1 + sizeof (PACKAGE) - 1
		+ sizeof (".cat");

  if (old_val != NULL && old_val[0] != '\0')
    new_val_len += strlen (old_val);

  new_val = (char *) tar_xmalloc (new_val_len);

# ifdef HAVE_SETENV
#  if __STDC__
  stpcpy (stpcpy (stpcpy (new_val,
			  DEF_MSG_DOM_DIR "/%L/LC_MESSAGES/" PACKAGE ".cat"),
#  else  
  stpcpy (stpcpy (stpcpy (stpcpy (stpcpy (stpcpy DEF_MSG_DOM_DIR),
					  "/%L/LC_MESSAGES/"),
				  PACKAGE),
			  ".cat"),
#  endif
	  	  old_val != NULL && old_val[0] != '\0' ? ":" : ""),
	  old_val != NULL && old_val[0] != '\0' ? old_val : "");

  setenv ("NLSPATH", new_val, 1);
# else
#  if __STDC__
  stpcpy (stpcpy (stpcpy (new_val,
			  "NLSPATH=" DEF_MSG_DOM_DIR "/%L/LC_MESSAGES/"
			  PACKAGE ".cat"),
#  else  
  stpcpy (stpcpy (stpcpy (stpcpy (stpcpy (stpcpy (stpcpy (new_val, "NLSPATH="),
						  DEF_MSG_DOM_DIR),
					  "/%L/LC_MESSAGES/"),
				  PACKAGE),
			  ".cat"),
#  endif
	  	  old_val != NULL && old_val[0] != '\0' ? ":" : ""),
	  old_val != NULL && old_val[0] != '\0' ? old_val : "");

  putenv (new_val);
# endif
#endif

  return (char *) domainname;
}

#undef gettext
char *
gettext (msg)
     const char *msg;
{
  int msgid;

  if (msg == NULL || catalog == (nl_catd) -1)
    return (char *) msg;

  /* Get the message from the catalog.  We always use set number 1.
     The message ID is computed by the function `msg_to_cat_id'
     which works on the table generated by `po-to-tbl'.  */
  msgid = msg_to_cat_id (msg);
  if (msgid == -1)
    return (char *) msg;

  return catgets (catalog, 1, msgid, (char *) msg);
}

/* Look through the table `_msg_tbl' which has `_msg_tbl_length' entries
   for the one equal to msg.  If it is found return the ID.  In case when
   the string is not found return -1.  */
static int
msg_to_cat_id (const char *msg)
{
  int cnt;

  for (cnt = 0; cnt < _msg_tbl_length; ++cnt)
    if (strcmp (msg, _msg_tbl[cnt]._msg) == 0)
      return _msg_tbl[cnt]._msg_number;

  return -1;
}
