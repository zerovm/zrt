/* gettextP.h -- header describing internals of gettext library
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

#ifndef _GETTEXTP_H
#define _GETTEXTP_H

/* @@ end of prolog @@ */

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif

#ifndef W
# define W(flag, data) ((flag) ? SWAP (data) : (data))
#endif

static inline nls_uint32
SWAP (i)
     nls_uint32 i;
{
  return (i << 24) | ((i & 0xff00) << 8) | ((i >> 8) & 0xff00) | (i >> 24);
}


struct loaded_domain
{
  struct loaded_domain *next;
  const char *data;
  const char *filename;
  int must_swap;
  nls_uint32 nstrings;
  struct string_desc *orig_tab;
  struct string_desc *trans_tab;
  nls_uint32 hash_size;
  nls_uint32 *hash_tab;
};

struct binding
{
  struct binding *next;
  char *domainname;
  char *dirname;
};

struct loaded_domain *_nl_find_domain __P ((const char *domainname,
					    int category));
struct loaded_domain *_nl_load_msg_cat __P ((const char *filename));

/* @@ begin of epilog @@ */

#endif /* gettextP.h  */
