/* Update a tar archive.
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

/* JF implement the 'r' 'u' and 'A' options for tar. */
/* The 'A' option is my own invention:  It means that the file-names are
   tar files, and they should simply be appended to the end of the archive.
   No attempt is made to block the reads from the args; if they're on raw
   tape or something like that, it'll probably lose. . . */

#include "system.h"

#ifndef	__MSDOS__
# include <pwd.h>
# include <grp.h>
#endif

#define STDIN 0
#define STDOUT 1

#include "tar.h"
#include "rmt.h"

/* We've hit the end of the old stuff, and its time to start writing new
   stuff to the tape.  This involves seeking back one block and
   re-writing the current block (which has been changed).  */
int time_to_start_writing = 0;

/* Pointer to where we started to write in the first block we write out.
   This is used if we can't backspace the output and have to null out the
   first part of the block.  */
char *output_start;

extern void skip_file ();	/* FIXME */
extern void skip_extended_headers (); /* FIXME */

extern union record *head;
extern struct stat hstat;

static void append_file __P ((char *));
static int move_arch __P ((int));
static void write_block __P ((int));

/*-----------------------------------------------------------------------.
  | Implement the 'r' (add files to end of archive), and 'u' (add files to |
  | end of archive if they arent there, or are more up to date than the	 |
  | version in the archive.) commands.					 |
  `-----------------------------------------------------------------------*/

void
update_archive (void)
{
    int found_end = 0;
    int status = 3;
    int prev_status;
    char *p;
    struct name *name;

    name_gather ();
    if (command_mode == COMMAND_UPDATE)
	name_expand ();
    DEBUG_PRINT("upd1");
    open_tar_archive (2);		/* open for updating */
    DEBUG_PRINT("upd2");
  
    do
	{
	    prev_status = status;
	    status = read_header ();
	    switch (status)
		{
		case EOF:
		    found_end = 1;
		    break;

		case 0:			/* a bad record */
		    userec (head);
		    switch (prev_status)
			{
			case 3:
			    WARN ((0, 0, _("This does not look like a tar archive")));
			    /* Fall through.  */

			case 2:
			case 1:
			    ERROR ((0, 0, _("Skipping to next header")));
			    /* Fall through.  */

			case 0:
			    break;
			}
		    break;

		case 1:			/* a good record */
#if 0
		    printf (_("File %s\n"), head->header.name);
		    head->header.name[NAMSIZ-1] = '\0';
#endif
		    if (command_mode == COMMAND_UPDATE
			&& (name = name_scan (current_file_name), name))
			{
#if 0
			    struct stat hstat;
#endif
			    struct stat nstat;
			    int unused;

			    decode_header (head, &hstat, &unused, 0);
			    if (stat (current_file_name, &nstat) < 0)
				ERROR ((0, errno, _("Cannot stat %s"), current_file_name));
			    else if (hstat.st_mtime >= nstat.st_mtime)
				name->found++;
			}
		    userec (head);
		    if (head->header.isextended)
			skip_extended_headers ();
		    skip_file ((long) hstat.st_size);
		    break;

		case 2:
		    ar_record = head;
		    found_end = 1;
		    break;
		}
	}
    while (!found_end);

    DEBUG_PRINT("upd3");
    reset_eof ();
    DEBUG_PRINT("upd3.1");
    time_to_start_writing = 1;
    output_start = ar_record->charptr;
    DEBUG_PRINT("upd3.2");
    while (p = name_from_list (), p)
	{
	    DEBUG_PRINT("upd4.0");
	    if (flag_confirm && !confirm ("add", p))
		continue;
	    if (command_mode == COMMAND_CAT){
		DEBUG_PRINT("upd4.1");
		append_file (p);
		DEBUG_PRINT("upd4.2");
	    }
	    else{
		DEBUG_PRINT("upd4.3");
		dump_file (p, -1, 1);
		DEBUG_PRINT("upd4.4");
	    }
	}
    DEBUG_PRINT("upd5");
    write_eot ();
    close_tar_archive ();
    names_notfound ();
}

/*-------------------------------------------------------------------------.
  | Catenate file p to the archive without creating a header for it.  It had |
  | better be a tar file or the archive is screwed.			   |
  `-------------------------------------------------------------------------*/

static void
append_file (char *p)
{
    int fd;
    struct stat statbuf;
    long bytes_left;
    union record *start;
    long bufsiz, count;

    if (stat (p, &statbuf) != 0 || (fd = open (p, O_RDONLY | O_BINARY), fd < 0))
	{
	    ERROR ((0, errno, _("Cannot open file %s"), p));
	    return;
	}

    bytes_left = statbuf.st_size;

    while (bytes_left > 0)
	{
	    start = findrec ();
	    bufsiz = endofrecs ()->charptr - start->charptr;
	    if (bytes_left < bufsiz)
		{
		    bufsiz = bytes_left;
		    count = bufsiz % RECORDSIZE;
		    if (count)
			memset (start->charptr + bytes_left, 0,
				(size_t) (RECORDSIZE - count));
		}
	    count = read (fd, start->charptr, (size_t) bufsiz);
	    if (count < 0)
		ERROR ((TAREXIT_FAILURE, errno,
			_("Read error at byte %ld reading %d bytes in file %s"),
			statbuf.st_size - bytes_left, bufsiz, p));
	    bytes_left -= count;
	    userec (start + (count - 1) / RECORDSIZE);
	    if (count != bufsiz)
		ERROR ((TAREXIT_FAILURE, 0, _("%s: File shrunk by %d bytes, (yark!)"),
			p, bytes_left));
	}
    close (fd);
}

#ifdef DONTDEF

/*---.
  | ?  |
  `---*/

static void
bprint (FILE *fp, char *buf, int num)
{
    int c;

    if (num == 0 || num == -1)
	return;
    fputs (" '", fp);
    while (num--)
	{
	    c = *buf++;
	    if (c == '\\')
		fputs ("\\\\", fp);
	    else if (c >= ' ' && c <= '~')
		putc (c, fp);
	    else
		switch (c)
		    {
		    case '\n':
			fputs ("\\n", fp);
			break;

		    case '\r':
			fputs ("\\r", fp);
			break;

		    case '\b':
			fputs ("\\b", fp);
			break;

		    case '\0':
#if 0
			fputs("\\-", fp);
#endif
			break;

		    default:
			fprintf (fp, "\\%03o", c);
			break;
		    }
	}
    fputs ("'\n", fp);
}

#endif

int number_of_blocks_read = 0;

int number_of_new_records = 0;
int number_of_records_needed = 0;

union record *new_block = 0;
union record *save_block = 0;

/*---.
  | ?  |
  `---*/

void
junk_archive (void)
{
    int found_stuff = 0;
    int status = 3;
    int prev_status;
    struct name *name;

#if 0
    int dummy_head;
#endif
    int number_of_records_to_skip = 0;
    int number_of_records_to_keep = 0;
    int number_of_kept_records_in_block;
    int sub_status;

#if 0
    fprintf (stderr,_("Junk files\n"));
#endif
    name_gather ();
    open_tar_archive (2);

    while (!found_stuff)
	{
	    prev_status = status;
	    status = read_header ();
	    switch (status)
		{
		case EOF:
		    found_stuff = 1;
		    break;

		case 0:
		    userec (head);
		    switch (prev_status)
			{
			case 3:
			    WARN ((0, 0, _("This does not look like a tar archive")));
			    /* Fall through.  */

			case 2:
			case 1:
			    ERROR ((0, 0, _("Skipping to next header")));
			    /* Fall through.  */

			case 0:
			    break;
			}
		    break;

		case 1:
#if 0
		    head->header.name[NAMSIZ-1] = '\0';
		    fprintf (stderr, _("file %s\n"), head->header.name);
#endif
		    if (name = name_scan (current_file_name), !name)
			{
			    userec (head);
#if 0
			    fprintf (stderr, _("Skip %ld\n"), (long) (hstat.st_size));
#endif
			    if (head->header.isextended)
				skip_extended_headers ();
			    skip_file ((long) (hstat.st_size));
			    break;
			}
		    name->found = 1;
		    found_stuff = 2;
		    break;

		case 2:
		    found_stuff = 1;
		    break;
		}
	}
#if 0
    fprintf (stderr, _("Out of first loop\n"));
#endif

    if (found_stuff != 2)
	{
	    write_eot ();
	    close_tar_archive ();
	    names_notfound ();
	    return;
	}

    if (write_archive_to_stdout)
	write_archive_to_stdout = 0;
    new_block = (union record *) tar_xmalloc ((size_t) blocksize);

    /* Save away records before this one in this block.  */

    number_of_new_records = ar_record - ar_block;
    number_of_records_needed = blocking - number_of_new_records;
    if (number_of_new_records)
	memcpy ((void *) new_block, (void *) ar_block,
		(size_t) (number_of_new_records * RECORDSIZE));

#if 0
    fprintf (stderr, _("Saved %d recs, need %d more\n"),
	     number_of_new_records, number_of_records_needed);
#endif
    userec (head);
    if (head->header.isextended)
	skip_extended_headers ();
    skip_file ((long) (hstat.st_size));
    found_stuff = 0;
#if 0
    goto flush_file;
#endif

    while (1)
	{

	    /* Fill in a block.  */

	    /* another_file:  */

	    if (ar_record == ar_last)
		{
#if 0
		    fprintf (stderr, _("New block\n"));
#endif
		    flush_archive ();
		    number_of_blocks_read++;
		}
	    sub_status = read_header ();
#if 0
	    fprintf (stderr, _("Header type %d\n"), sub_status);
#endif

	    if (sub_status == 2 && flag_ignorez)
		{
		    userec (head);
		    continue;
		}
	    if (sub_status == EOF || sub_status == 2)
		{
		    found_stuff = 1;
		    memset (new_block[number_of_new_records].charptr, 0,
			    (size_t) (RECORDSIZE * number_of_records_needed));
		    number_of_new_records += number_of_records_needed;
		    number_of_records_needed = 0;
		    write_block (0);
		    break;
		}

	    if (sub_status == 0)
		{
		    ERROR ((0, 0, _("Deleting non-header from archive")));
		    userec (head);
		    continue;
		}

	    /* Found another header.  Yipee!  */

#if 0
	    head->header.name[NAMSIZ-1] = '\0';
	    fprintf (stderr, _("File %s "), head->header.name);
#endif
	    if (name = name_scan (current_file_name), name)
		{
		    name->found = 1;
#if 0
		    fprintf (stderr, _("Flush it\n"));
		flush_file:
		    decode_header (head, &hstat,&dummy_head, 0);
#endif
		    userec (head);
		    number_of_records_to_skip = (hstat.st_size + RECORDSIZE - 1) / RECORDSIZE;
#if 0
		    fprintf (stderr, _("Flushing %d recs from %s\n"),
			     number_of_records_to_skip,head->header.name);
#endif

		    while (ar_last - ar_record <= number_of_records_to_skip)
			{
#if 0
			    fprintf (stderr, _("Block: %d <= %d  "),
				     ar_last-ar_record, number_of_records_to_skip);
#endif
			    number_of_records_to_skip -= (ar_last - ar_record);
			    flush_archive ();
			    number_of_blocks_read++;
#if 0
			    fprintf (stderr, _("Block %d left\n"),
				     number_of_records_to_skip);
#endif
			}
		    ar_record += number_of_records_to_skip;
#if 0
		    fprintf (stderr, _("Final %d\n"), number_of_records_to_skip);
#endif
		    number_of_records_to_skip = 0;
		    continue;
		}

	    /* copy_header:  */

	    new_block[number_of_new_records] = *head;
	    number_of_new_records++;
	    number_of_records_needed--;
	    number_of_records_to_keep
		= (hstat.st_size + RECORDSIZE - 1) / RECORDSIZE;
	    userec (head);
	    if (number_of_records_needed == 0)
		write_block (1);

	    /* copy_data: */

	    number_of_kept_records_in_block = ar_last - ar_record;
	    if (number_of_kept_records_in_block > number_of_records_to_keep)
		number_of_kept_records_in_block = number_of_records_to_keep;

#if 0
	    fprintf (stderr, _("Need %d kept_in %d keep %d\n"),
		     blocking, number_of_kept_records_in_block,
		     number_of_records_to_keep);
#endif

	    while (number_of_records_to_keep)
		{
		    int n;

		    if (ar_record == ar_last)
			{
#if 0
			    fprintf (stderr, _("Flush...\n"));
#endif
			    fl_read ();
			    number_of_blocks_read++;
			    ar_record = ar_block;
			    number_of_kept_records_in_block = blocking;
			    if (number_of_kept_records_in_block > number_of_records_to_keep)
				number_of_kept_records_in_block = number_of_records_to_keep;
			}
		    n = number_of_kept_records_in_block;
		    if (n > number_of_records_needed)
			n = number_of_records_needed;

#if 0
		    fprintf (stderr, _("Copying %d\n"), n);
#endif
		    memcpy ((void *) (new_block + number_of_new_records),
			    (void *) ar_record,
			    (size_t) (n * RECORDSIZE));
		    number_of_new_records += n;
		    number_of_records_needed -= n;
		    ar_record += n;
		    number_of_records_to_keep -= n;
		    number_of_kept_records_in_block -= n;
#if 0
		    fprintf (stderr,
			     _("Now new %d  need %d  keep %d  keep_in %d rec %d/%d\n"),
			     number_of_new_records, number_of_records_needed,
			     number_of_records_to_keep, number_of_kept_records_in_block,
			     ar_record-ar_block, ar_last-ar_block);
#endif

		    if (number_of_records_needed == 0)
			{
			    write_block (1);
			}
		}
	}

    write_eot ();
    close_tar_archive ();
    names_notfound ();
}

/*---.
  | ?  |
  `---*/

static void
write_block (int f)
{
#if 0
    fprintf (stderr, _("Write block\n"));
#endif

    /* We've filled out a block.  Write it out.  Backspace back to where we
       started.  */

    if (archive != STDIN)
	move_arch (-(number_of_blocks_read + 1));

    save_block = ar_block;
    ar_block = new_block;

    if (archive == STDIN)
	archive = STDOUT;
    fl_write ();

    if (archive == STDOUT)
	archive = STDIN;
    ar_block = save_block;

    if (f)
	{

	    /* Move the tape head back to where we were.  */

	    if (archive != STDIN)
		move_arch (number_of_blocks_read);
	    number_of_blocks_read--;
	}

    number_of_records_needed = blocking;
    number_of_new_records = 0;
}

/*-----------------------------------------------------------------------.
  | Move archive descriptor by n blocks worth.  If n is positive we move	 |
  | forward, else we move negative.  If its a tape, MTIOCTOP had better	 |
  | work.  If its something else, we try to seek on it.  If we can't seek, |
  | we lose!								 |
  `-----------------------------------------------------------------------*/

static int
move_arch (int n)
{
    off_t cur;

#ifdef MTIOCTOP
    struct mtop t;
    int er;

    if (n > 0)
	{
	    t.mt_op = MTFSR;
	    t.mt_count = n;
	}
    else
	{
	    t.mt_op = MTBSR;
	    t.mt_count = -n;
	}
    if (er = rmtioctl (archive, MTIOCTOP, (char *) &t), er >= 0)
	return 1;
    if (errno == EIO
	&& (er = rmtioctl (archive, MTIOCTOP, (char *) &t), er >= 0))
	return 1;
#endif

    cur = rmtlseek (archive, 0L, 1);
    cur += blocksize * n;

#if 0
    fprintf (stderr, _("Fore to %x\n"), cur);
#endif
    if (rmtlseek (archive, cur, 0) != cur)
	/* Lseek failed.  Try a different method.  */
	ERROR ((TAREXIT_FAILURE, 0, _("Could not re-position archive file")));

    return 3;
}
