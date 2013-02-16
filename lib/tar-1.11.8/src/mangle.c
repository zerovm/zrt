/* mangle.c -- encode long filenames
   Copyright (C) 1988, 1992, 1994 Free Software Foundation, Inc.

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

#include "tar.h"

extern struct stat hstat;	/* stat struct corresponding */

struct mangled
  {
    struct mangled *next;
    int type;
    char mangled[NAMSIZ];
    char *linked_to;
    char normal[1];
  };

/* Should use a hash table, etc. .  */
struct mangled *first_mangle;
int mangled_num = 0;

/*---.
| ?  |
`---*/

void
extract_mangle (void)
{
  char *buf;
  char *fromtape;
  char *to;
  char *ptr, *ptrend;
  char *nam1, *nam1end;
  int size;
  int copied;

  size = hstat.st_size;
  buf = to = xmalloc ((size_t) (size + 1));
  buf[size] = '\0';
  while (size > 0)
    {
      fromtape = findrec ()->charptr;
      if (fromtape == 0)
	{
	  ERROR ((0, 0, _("Unexpected EOF in mangled names")));
	  return;
	}
      copied = endofrecs ()->charptr - fromtape;
      if (copied > size)
	copied = size;
      memcpy (to, fromtape, (size_t) copied);
      to += copied;
      size -= copied;
      userec ((union record *) (fromtape + copied - 1));
    }
  for (ptr = buf; *ptr; ptr = ptrend)
    {
      ptrend = strchr (ptr, '\n');
      *ptrend++ = '\0';

      if (!strncmp (ptr, "Rename ", 7))
	{
	  nam1 = ptr + 7;
	  nam1end = strchr (nam1, ' ');
	  while (strncmp (nam1end, " to ", 4))
	    {
	      nam1end++;
	      nam1end = strchr (nam1end, ' ');
	    }
	  *nam1end = '\0';
	  if (ptrend[-2] == '/')
	    ptrend[-2] = '\0';
	  un_quote_string (nam1end + 4);
	  if (rename (nam1, nam1end + 4))
	    ERROR ((0, errno, _("Cannot rename %s to %s"), nam1, nam1end + 4));
	  else if (flag_verbose)
	    WARN ((0, 0, _("Renamed %s to %s"), nam1, nam1end + 4));
	}
#ifdef S_ISLNK
      else if (!strncmp (ptr, "Symlink ", 8))
	{
	  nam1 = ptr + 8;
	  nam1end = strchr (nam1, ' ');
	  while (strncmp (nam1end, " to ", 4))
	    {
	      nam1end++;
	      nam1end = strchr (nam1end, ' ');
	    }
	  *nam1end = '\0';
	  un_quote_string (nam1);
	  un_quote_string (nam1end + 4);
	  if (symlink (nam1, nam1end + 4)
	      && (unlink (nam1end + 4) || symlink (nam1, nam1end + 4)))
	    ERROR ((0, errno, _("Cannot symlink %s to %s"),
		    nam1, nam1end + 4));
	  else if (flag_verbose)
	    WARN ((0, 0, _("Symlinked %s to %s"), nam1, nam1end + 4));
	}
#endif
      else
	ERROR ((0, 0, _("Unknown demangling command %s"), ptr));
    }
}
