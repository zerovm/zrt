/* Generate a file containing some preset patterns.
   Copyright (C) 1995 Free Software Foundation, Inc.
   François Pinard <pinard@iro.umontreal.ca>, 1995.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "system.h"

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

/* The name this program was run with. */
const char *program_name;

/* If non-zero, display usage information and exit.  */
static int show_help = 0;

/* If non-zero, print the version on standard output and exit.  */
static int show_version = 0;

/* Length of file to generate.  */
static int file_length = 0;

/*-----------------------------------------------.
| Explain how to use the program, then get out.	 |
`-----------------------------------------------*/

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf ("GNU %s %s\n", PACKAGE, VERSION);
      printf (_("\
\n\
Usage: %s [OPTION]...\n"), program_name);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
\n\
      --help            display this help and exit\n\
      --version         output version information and exit\n"),
	     stdout);
    }
  exit (status);
}

/*----------------------------------------------------------------------.
| Main program.  Decode ARGC arguments passed through the ARGV array of |
| strings, then launch execution.				        |
`----------------------------------------------------------------------*/

/* Long options equivalences.  */
static const struct option long_options[] =
{
  {"help", no_argument, &show_help, 1},
  {"length", required_argument, NULL, 'l'},
  {"version", no_argument, &show_version, 1},
  {0, 0, 0, 0},
};

int
zmain (int argc, char *const *argv)
{
  int option_char;		/* option character */
  int counter;			/* general purpose counter */

  /* Decode command options.  */

  program_name = argv[0];
  setlocale (LC_ALL, "");

  while (option_char = getopt_long (argc, argv, "l:", long_options, NULL),
	 option_char != EOF)
    switch (option_char)
      {
      default:
	usage (EXIT_FAILURE);

      case '\0':
	break;

      case 'l':
	file_length = atoi (optarg);
	break;
      }

  /* Process trivial options.  */

  if (show_version)
    {
      printf ("GNU %s %s\n", PACKAGE, VERSION);
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    usage (EXIT_SUCCESS);

  if (optind < argc)
    usage (EXIT_FAILURE);

  /* Generate file.  */

  for (counter = 0; counter < file_length; counter++)
    putchar (counter & 255);

  exit (0);
}
