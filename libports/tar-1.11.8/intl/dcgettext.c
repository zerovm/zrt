/* dcgettext.c -- implemenatation of the dcgettext(3) function
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

#include <string.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "gettext.h"
#include "gettextP.h"
#include "libgettext.h"
#include "hash-string.h"

/* @@ end of prolog @@ */

/* Name of the default domain used for gettext(3) prior any call to
   textdomain(3).  The default value for this is "messages".  */
const char _nl_default_default_domain[] = "messages";

/* Value used as the default domain for gettext(3).  */
const char *_nl_current_default_domain = _nl_default_default_domain;


/* Look up MSGID in the DOMAINNAME message catalog for the current CATEGORY
   locale.  */
char *
dcgettext (domainname, msgid, category)
     const char *domainname;
     const char *msgid;
     int category;
{
  struct loaded_domain *domain;
  size_t top, act, bottom;
  int cmp_val;

  /* If no real MSGID is given return NULL.  */
  if (msgid == NULL)
    return NULL;

  /* If DOMAINNAME is NULL, we are interested in the default domain.  If
     CATEGORY is not LC_MESSAGES this might not make much sense but the
     defintion left this undefined.  */
  if (domainname == NULL)
    domainname = _nl_current_default_domain;

  /* Find structure describing the message catalog matching the
     DOMAINNAME and CATEGORY.  */
  domain = _nl_find_domain (domainname, category);

  if (domain == NULL || domain->data == NULL)
    /* No message catalog found.  */
    return (char *) msgid;

  /* Locate the MSGID and its translation.  */
  if (domain->hash_size > 0 && domain->hash_tab != NULL)
    {
      /* Use the hashing table.  */
      nls_uint32 len = strlen (msgid);
      nls_uint32 hash_val = hash_string (msgid);
      nls_uint32 idx = hash_val % domain->hash_size;
      nls_uint32 incr = 1 + (hash_val % (domain->hash_size - 2));
      nls_uint32 nstr = W (domain->must_swap, domain->hash_tab[idx]);

      if (nstr == 0)
        /* Hash table entry is empty.  */
        return (char *) msgid;

      if (W (domain->must_swap, domain->orig_tab[nstr - 1].length) == len
          && strcmp (msgid,
		     domain->data + W (domain->must_swap,
				       domain->orig_tab[nstr - 1].offset)) == 0)
	  return (char *) domain->data + W (domain->must_swap,
					    domain->trans_tab[nstr - 1].offset);

      while (1)
	{
	  if (idx >= W (domain->must_swap, domain->hash_size) - incr)
	    idx -= W (domain->must_swap, domain->hash_size) - incr;
	  else
	    idx += incr;

	  nstr = W (domain->must_swap, domain->hash_tab[idx]);
	  if (nstr == 0)
	    /* Hash table entry is empty.  */
	    return (char *) msgid;

	  if (W (domain->must_swap, domain->orig_tab[nstr - 1].length) == len
	      && strcmp (msgid,
			 domain->data + W (domain->must_swap,
					   domain->orig_tab[nstr - 1].offset))
		 == 0)
	    return (char *) domain->data
		   + W (domain->must_swap, domain->trans_tab[nstr - 1].offset);
	}
      /* NOTREACHED */
    }

  /* Now we try the default method:  binary search in the sorted
     array of messages.  */
  bottom = 0;
  top = domain->nstrings;
  while (bottom < top)
    {
      act = (bottom + top) / 2;
      cmp_val = strcmp (msgid, domain->data + W (domain->must_swap,
						 domain->orig_tab[act].offset));
      if (cmp_val < 0)
	top = act;
      else if (cmp_val > 0)
	bottom = act + 1;
      else
	break;
    }

  return bottom >= top ? (char *) msgid
	 : (char *) domain->data
	   + W (domain->must_swap, domain->trans_tab[act].offset);
}
