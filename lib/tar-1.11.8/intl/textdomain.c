/* textdomain.c -- implementation of the textdomain(3) function
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

#include <stdlib.h>
#include <string.h>

#include "libgettext.h"

/* @@ end of prolog @@ */

/* Prototypes for functions of libnlsut.  */
extern char *xstrdup __P((const char *__str));

/* Name of the default text domain.  */
extern const char _nl_default_default_domain[];

/* Default text domain in which entries for gettext(3) are to be found.  */
extern const char *_nl_current_default_domain;


/* Set the current default message catalog to DOMAINNAME.
   If DOMAINNAME is null, return the current default.
   If DOMAINNAME is "", reset to the default of "messages".  */
char *
textdomain (domainname)
     const char *domainname;
{
  char *old;

  /* A NULL pointer requests the current setting.  */ 
  if (domainname == NULL)
    return (char *) _nl_current_default_domain;

  old = (char *) _nl_current_default_domain;

  /* If domain name is the null string set to default domain "messages".  */
  if (domainname[0] == '\0'
      || strcmp (domainname, _nl_default_default_domain) == 0)
    _nl_current_default_domain = _nl_default_default_domain;
  else
    _nl_current_default_domain = xstrdup (domainname);

  if (old != _nl_default_default_domain)
    free (old);

  return (char *) _nl_current_default_domain;
}
