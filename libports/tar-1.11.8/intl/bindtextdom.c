/* bindtextdom.c -- implementation of the bindtextdomain(3) function
   Copyright (C) 1995 Free Software Foundation, Inc.

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

#include <libgettext.h>
#include <stdlib.h>
#include <string.h>

#include "gettext.h"
#include "gettextP.h"

/* @@ end of prolog @@ */

/* Contains the default location of the message catalogs.  */
extern const char _nl_default_dirname[];

/* List with bindings of specific domains.  */
extern struct binding *_nl_domain_bindings;

/* Prototypes for library functions.  */
void *tar_xmalloc ();
char *xstrdup ();


/* Specify that the DOMAINNAME message catalog will be found
   in DIRNAME rather than in the system locale data base.  */
char *
bindtextdomain (domainname, dirname)
     const char *domainname;
     const char *dirname;
{
  struct binding *binding;

  /* Some sanity checks.  */
  if (domainname == NULL || domainname[0] == '\0')
    return NULL;

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

  if (dirname == NULL)
    /* The current binding has be to returned.  */
    return binding == NULL ? (char *) _nl_default_dirname : binding->dirname;

  if (binding != NULL)
    {
      /* The domain is already bound.  Replace the old binding.  */
      char *new_dirname = strcmp (dirname, _nl_default_dirname) == 0
			  ? (char *) _nl_default_dirname : xstrdup (dirname);

      if (strcmp (binding->dirname, _nl_default_dirname) != 0)
        free (binding->dirname);

      binding->dirname = new_dirname;
    }
  else
    {
      /* We have to create a new binding.  */
      struct binding *new_binding =
	(struct binding *) tar_xmalloc (sizeof (*new_binding));


      new_binding->domainname = xstrdup (domainname);
      new_binding->dirname = strcmp (dirname, _nl_default_dirname) == 0
			     ? (char *) _nl_default_dirname : xstrdup (dirname);

      /* Now enqueue it.  */
      if (_nl_domain_bindings == NULL
	  || strcmp (domainname, _nl_domain_bindings->domainname) < 0)
	{
	  new_binding->next = _nl_domain_bindings;
	  _nl_domain_bindings = new_binding;
	}
      else
	{
	  binding = _nl_domain_bindings;
	  while (binding->next != NULL
		 && strcmp (domainname, binding->next->domainname) > 0)
	    binding = binding->next;

	  new_binding->next = binding->next;
	  binding->next = new_binding;
	}

      binding = new_binding;
    }

  return binding->dirname;
}
