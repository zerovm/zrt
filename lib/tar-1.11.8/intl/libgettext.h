/* libgettext.h -- Message catalogs for internationalization.
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

#ifndef _LIBINTL_H
#define	_LIBINTL_H	1

#include <locale.h>

/* @@ end of prolog @@ */

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif

#ifndef HAVE_LC_MESSAGES
/* This value determines the behaviour of the gettext() and dgettext()
   function.  But some system does not have this defined.  Define it
   to a default value.  */
# define LC_MESSAGES (-1)
#endif

/* Declarations for gettext-using-catgets interface.  Derived from
   Jim Meyering's libintl.h.  */

struct _msg_ent
{
  const char *_msg;
  int _msg_number;
};

/* These two variables are defined in the automatically by po-to-tbl.sed
   generated file `cat-id-tbl.c'.  */
extern const struct _msg_ent _msg_tbl[];
extern int _msg_tbl_length;


/* Look up MSGID in the current default message catalog for the current
   LC_MESSAGES locale.  If not found, returns MSGID itself (the default
   text).  */
extern char *gettext __P ((const char *__msgid));

/* Look up MSGID in the DOMAINNAME message catalog for the current
   LC_MESSAGES locale.  */
extern char *dgettext __P ((const char *__domainname, const char *__msgid));

/* Look up MSGID in the DOMAINNAME message catalog for the current CATEGORY
   locale.  */
extern char *dcgettext __P ((const char *__domainname, const char *__msgid,
			     int __category));


/* Set the current default message catalog to DOMAINNAME.
   If DOMAINNAME is null, return the current default.
   If DOMAINNAME is "", reset to the default of "messages".  */
extern char *textdomain __P ((const char *__domainname));

/* Specify that the DOMAINNAME message catalog will be found
   in DIRNAME rather than in the system locale data base.  */
extern char *bindtextdomain __P ((const char *__domainname,
				  const char *__dirname));

#if defined ENABLE_NLS

# if !defined HAVE_CATGETS

#  define gettext(msgid)       	     dgettext (NULL, msgid)

#  define dgettext(domainname, msgid) \
                                     dcgettext (domainname, msgid, LC_MESSAGES)

#  if defined __GNUC__ && __GNUC__ == 2 && __GNUC_MINOR__ >= 7
#   define	dcgettext(domainname, msgid, category)			      \
  (__extension__							      \
   ({									      \
     if (__builtin_constant_p (msgid))					      \
       {								      \
	 extern int _nl_msg_cat_cntr;					      \
	 static char *__translation__;					      \
	 static int __catalog_counter__;				      \
	 if (! __translation__ || __catalog_counter__ != _nl_msg_cat_cntr)    \
	   {								      \
	     __translation__ =						      \
	       (dcgettext) ((domainname), (msgid), (category));		      \
	     __catalog_counter__ = _nl_msg_cat_cntr;			      \
	   }								      \
	 __translation__;						      \
       }								      \
     else								      \
       (dcgettext) ((domainname), (msgid), (category));			      \
    }))
#  endif
# endif

#else

# define gettext(msgid) (msgid)
# define dgettext(domainname, msgid) (msgid)
# define dcgettext(domainname, msgid, category) (msgid)
# define textdomain(domainname) while (0) /* nothing */
# define bindtextdomain(domainname, dirname) while (0) /* nothing */

#endif

/* @@ begin of epilog @@ */

#endif	/* libgettext.h */
