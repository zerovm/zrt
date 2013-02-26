/* loadmsgcat.c -- load needed message catalogs
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

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_MMAP
# include <sys/mman.h>
#endif

#include "gettext.h"
#include "gettextP.h"

/* @@ end of prolog @@ */

/* List of already loaded domains.  */
struct loaded_domain *_nl_loaded_domains;

/* We need a sign, whether a new catalog was loaded, which can be associated
   with all translations.  This is important if the translations are
   cached by one of GCC's features.  */
int _nl_msg_cat_cntr;


/* Prototypes for library functions.  */
void *xmalloc ();

/* Load the message catalogs specified by FILENAME.  If it is no valid
   message catalog return null.  */
struct loaded_domain *
_nl_load_msg_cat (filename)
     const char *filename;
{
  int fd;
  struct stat st;
  struct loaded_domain *retval;
  struct mo_file_header *data = (struct mo_file_header *) -1;
  int use_mmap = 0;

  /* We will create an entry for each file.  Those representing a
     non-existing or illegal file have the DATA member set to null.
     This helps subsequent calls to detect this situation without
     trying to load.  */
  retval = (struct loaded_domain *) xmalloc (sizeof (*retval));
  /* Note: FILENAME is allocated in finddomain and can be used here.  */
  retval->filename = filename;
  retval->data = NULL;

  /* Show that one domain is changed.  This might make some cached
     translation invalid.  */
  ++_nl_msg_cat_cntr;

  /* Enqueue the new entry.  */
  if (_nl_loaded_domains == NULL
      || strcmp (filename, _nl_loaded_domains->filename) < 0)
    {
      retval->next = _nl_loaded_domains;
      _nl_loaded_domains = retval;
    }
  else
    {
      struct loaded_domain *rp = _nl_loaded_domains;

      while (rp->next != NULL && strcmp (filename, rp->next->filename) > 0)
	rp = rp->next;

      retval->next = rp->next;
      rp->next = retval;
    }

  /* Try to open the addressed file.  */
  fd = open (filename, O_RDONLY);
  if (fd == -1)
    return retval;

  /* We must know about the size of the file.  */
  if (fstat (fd, &st) != 0
      && st.st_size < (off_t) sizeof (struct mo_file_header))
    {
      /* Something went wrong.  */
      close (fd);
      return retval;
    }

#ifdef HAVE_MMAP
  /* Now we are ready to load the file.  If mmap() is available we try
     this first.  If not available or it failed we try to load it.  */
  data = (struct mo_file_header *) mmap (NULL, st.st_size, PROT_READ,
					 MAP_PRIVATE, fd, 0);

  if (data != (struct mo_file_header *) -1)
    {
      /* mmap() call was successful.  */
      close (fd);
      use_mmap = 1;
    }
#endif

  /* If the data is not yet available (i.e. mmap'ed) we try to load
     it manually.  */
  if (data == (struct mo_file_header *) -1)
    {
      off_t to_read;
      char *read_ptr;

      data = (struct mo_file_header *) xmalloc (st.st_size);

      to_read = st.st_size;
      read_ptr = (char *) data;
      do
	{
	  long int nb = (long int) read (fd, read_ptr, to_read);
	  if (nb == -1)
	    {
	      close (fd);
	      return retval;
	    }

	  read_ptr += nb;
	  to_read -= nb;
	}
      while (to_read > 0);

      close (fd);
    }

  /* Using the magic number we can test whether it really is a message
     catalog file.  */
  if (data->magic != _MAGIC && data->magic != _MAGIC_SWAPPED)
    {
      /* The magic number is wrong: not a message catalog file.  */
      if (use_mmap)
	munmap ((caddr_t) data, st.st_size);
      else
	free (data);
      return retval;
    }

  retval->data = (char *) data;
  retval->must_swap = data->magic != _MAGIC;

  /* Fill in the information about the available tables.  */
  switch (W (retval->must_swap, data->revision))
    {
    case 0:
      retval->nstrings = W (retval->must_swap, data->nstrings);
      retval->orig_tab = (struct string_desc *)
	((char *) data + W (retval->must_swap, data->orig_tab_offset));
      retval->trans_tab = (struct string_desc *)
	((char *) data + W (retval->must_swap, data->trans_tab_offset));
      retval->hash_size = W (retval->must_swap, data->hash_tab_size);
      retval->hash_tab = (nls_uint32 *)
	((char *) data + W (retval->must_swap, data->hash_tab_offset));
      break;
    default:
      /* This is an illegal revision.  */
      if (use_mmap)
	munmap ((caddr_t) data, st.st_size);
      else
	free (data);
      retval->data = NULL;
    }

  return retval;
}
