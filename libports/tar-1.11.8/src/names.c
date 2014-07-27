/* Look up user and/or group names.
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

/* Look up user and/or group names.

   This file should be modified for non-unix systems to do something
   reasonable.  */

#include "system.h"

#ifndef NONAMES
/* Whole module goes away if NONAMES defined.  Otherwise... */

#include <pwd.h>
#include <grp.h>

extern struct group *getgrnam ();
extern struct passwd *getpwnam ();
#ifndef HAVE_GETPWUID
extern struct passwd *getpwuid ();
#endif

#include "tar.h"

static int saveuid = -993;
static char saveuname[TUNMLEN];
static int my_uid = -993;

static int savegid = -993;
static char savegname[TGNMLEN];
static int my_gid = -993;

#define myuid	( my_uid < 0? (my_uid = getuid()): my_uid )
#define	mygid	( my_gid < 0? (my_gid = getgid()): my_gid )

/*-------------------------------------------------------------------.
| Look up a user or group name from a uid/gid, maintaining a cache.  |
`-------------------------------------------------------------------*/

/* FIXME, for now it's a one-entry cache.  FIXME2, the "-993" is to
   reduce the chance of a hit on the first lookup.

   This is ifdef'd because on Suns, it drags in about 38K of "yellow
   pages" code, roughly doubling the program size.  Thanks guys.  */

void
__tar_finduname (char uname[TUNMLEN], int uid)
{
  struct passwd *pw;

  if (uid != saveuid)
    {
      saveuid = uid;
      saveuname[0] = '\0';
      pw = getpwuid (uid);
      if (pw)
	strncpy (saveuname, pw->pw_name, TUNMLEN);
    }
  strncpy (uname, saveuname, TUNMLEN);
}

/*---.
| ?  |
`---*/

#ifdef __ZRT__
int
__tar_finduid (char uname[TUNMLEN])
{
    saveuid = myuid;
    return saveuid;
}
#else
int
__tar_finduid (char uname[TUNMLEN])
{
  struct passwd *pw;

  if (uname[0] != saveuname[0]	/* quick test w/o proc call */
      || strncmp (uname, saveuname, TUNMLEN) != 0)
    {
      strncpy (saveuname, uname, TUNMLEN);
      pw = getpwnam (uname);
      if (pw)
	{
	  saveuid = pw->pw_uid;
	}
      else
	{
	  saveuid = myuid;
	}
    }
  return saveuid;
}
#endif //__ZRT__

/*---.
| ?  |
`---*/

void
__tar_findgname (char gname[TGNMLEN], int gid)
{
#ifndef __ZRT__
  struct group *gr;
#ifndef HAVE_GETGRGID
  extern struct group *getgrgid ();
#endif

  if (gid != savegid)
    {
      savegid = gid;
      savegname[0] = '\0';
      setgrent ();
      gr = getgrgid (gid);
      if (gr)
	strncpy (savegname, gr->gr_name, TGNMLEN);
    }
  strncpy (gname, savegname, TGNMLEN);
#endif
}

/*---.
| ?  |
`---*/

int
__tar_findgid (char gname[TUNMLEN])
{
#ifdef __ZRT__
    savegid = mygid;
    return savegid;
#else
  struct group *gr;

  if (gname[0] != savegname[0]	/* quick test w/o proc call */
      || strncmp (gname, savegname, TUNMLEN) != 0)
    {
      strncpy (savegname, gname, TUNMLEN);
      gr = getgrnam (gname);
      if (gr)
	{
	  savegid = gr->gr_gid;
	}
      else
	{
	  savegid = mygid;
	}
    }
  return savegid;
#endif //__ZRT__
}

#endif
