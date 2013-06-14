/* Declarations for tar archives.
   Copyright (C) 1988, 1992, 1993, 1994 Free Software Foundation, Inc.

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

/* Standard Archive Format - Standard TAR - USTAR.  */

/* Header block on tape.
   
   We use traditional DP naming conventions here.  A "block" is a big chunk
   of stuff that we do I/O on.  A "record" is a piece of info that we care
   about.  Typically many "record"s fit into a "block".  */

#define RECORDSIZE	512
#define NAMSIZ		100
#define TUNMLEN		32
#define TGNMLEN		32
#define SPARSE_EXT_HDR	21
#define SPARSE_IN_HDR	4

/*debugging is switched off, and also stderr WARN*/
#define SWITCH_OFF_DEBUG_PRINT

#ifdef SWITCH_OFF_DEBUG_PRINT
#  define DEBUG_PRINT(s)
#else
#  define DEBUG_PRINT(s) {fprintf(stderr, "%s\n",s );fflush(0);}
#endif

struct sparse
  {
    char offset[12];
    char numbytes[12];
  };

union record
  {
    char charptr[RECORDSIZE];

    struct header
      {
	char arch_name[NAMSIZ];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char linkflag;
	char arch_linkname[NAMSIZ];
	char magic[8];
	char uname[TUNMLEN];
	char gname[TGNMLEN];
	char devmajor[8];
	char devminor[8];

	/* The following fields were added for GNU and are not standard.  */

	char atime[12];
	char ctime[12];
	char offset[12];
	char longnames[4];
	/* Some compilers would insert the pad themselves, so pad was
	   once autoconfigured.  It is simpler to always insert it!  */
	char pad;
	struct sparse sp[SPARSE_IN_HDR];
	char isextended;
	char realsize[12];	/* true size of the sparse file */
#if 0
	char ending_blanks[12];	/* number of nulls at the end of the file,
				   if any */
#endif
      }
    header;

    struct extended_header
      {
	struct sparse sp[21];
	char isextended;
      }
    ext_hdr;
  };

/* The checksum field is filled with this while the checksum is computed.  */
#define CHKBLANKS	"        "	/* 8 blanks, no null */

/* The magic field is filled with this value if uname and gname are valid,
   marking the archive as being in standard POSIX format (though GNU tar
   itself is not POSIX conforming).  */
#define TMAGIC "ustar  "	/* 7 chars and a null */

/* The magic field is filled with this if this is a GNU format dump entry.
   But I suspect this is not true anymore.  */
#define GNUMAGIC "GNUtar "	/* 7 chars and a null */

/* The linkflag defines the type of file.  */
#define LF_OLDNORMAL	'\0'	/* normal disk file, Unix compat */
#define LF_NORMAL	'0'	/* normal disk file */
#define LF_LINK		'1'	/* link to previously dumped file */
#define LF_SYMLINK	'2'	/* symbolic link */
#define LF_CHR		'3'	/* character special file */
#define LF_BLK		'4'	/* block special file */
#define LF_DIR		'5'	/* directory */
#define LF_FIFO		'6'	/* FIFO special file */
#define LF_CONTIG	'7'	/* contiguous file */
/* Further link types may be defined later.  */

/* Note that the standards committee allows only capital A through
   capital Z for user-defined expansion.  This means that defining
   something as, say '8' is a *bad* idea.  */

/* This is a dir entry that contains the names of files that were in the
   dir at the time the dump was made.  */
#define LF_DUMPDIR	'D'

/* Identifies the NEXT file on the tape as having a long linkname.  */
#define LF_LONGLINK	'K'

/* Identifies the NEXT file on the tape as having a long name.  */
#define LF_LONGNAME	'L'

/* This is the continuation of a file that began on another volume.  */
#define LF_MULTIVOL	'M'

/* For storing filenames that didn't fit in 100 characters.  */
#define LF_NAMES	'N'

/* This is for sparse files.  */
#define LF_SPARSE	'S'

/* This file is a tape/volume header.  Ignore it on extraction.  */
#define LF_VOLHDR	'V'

#if 0
/* The following two blocks of #define's are unused in GNU tar.  */

/* Bits used in the mode field - values in octal */
#define  TSUID    04000		/* set UID on execution */
#define  TSGID    02000		/* set GID on execution */
#define  TSVTX    01000		/* save text (sticky bit) */

/* File permissions */
#define  TUREAD   00400		/* read by owner */
#define  TUWRITE  00200		/* write by owner */
#define  TUEXEC   00100		/* execute/search by owner */
#define  TGREAD   00040		/* read by group */
#define  TGWRITE  00020		/* write by group */
#define  TGEXEC   00010		/* execute/search by group */
#define  TOREAD   00004		/* read by other */
#define  TOWRITE  00002		/* write by other */
#define  TOEXEC   00001		/* execute/search by other */

#endif

/* End of Standard Archive Format description.  */

/* Kludge for handling systems that can't cope with multiple external
   definitions of a variable.  In ONE routine (tar.c), we #define GLOBAL to
   null; here, we set it to "extern" if it is not already set.  */

#ifndef GLOBAL
# define GLOBAL extern
#endif

/* Exit status for GNU tar.  Let's try to keep this list as simple as
   possible.  -d option strongly invites a different status for unequal
   comparison and other errors.  */

#define TAREXIT_SUCCESS 0
#define TAREXIT_DIFFERS 1
#define TAREXIT_FAILURE 2

/* Global variables.  */

struct sp_array
  {
    off_t offset;
    int numbytes;
  };

/* Start of block of archive.  */
GLOBAL union record *ar_block;

/* Current record of archive.  */
GLOBAL union record *ar_record;

/* Last+1 record of archive block.  */
GLOBAL union record *ar_last;

/* 0 writing, !0 reading archive.  */
GLOBAL char ar_reading;

/* Size of each block, in records.  */
GLOBAL int blocking;

/* Size of each block, in bytes.  */
GLOBAL int blocksize;

/* Script to run at end of each tape change.  */
GLOBAL char *info_script;

/* File containing names to work on.  */
GLOBAL const char *namefile_name;

/* \n or \0.  */
GLOBAL char filename_terminator;

/* Name of this program.  */
GLOBAL const char *program_name;

/* Pointer to the start of the scratch space.  */
GLOBAL struct sp_array *sparsearray;

/* Initial size of the sparsearray.  */
GLOBAL int sp_array_size;

/* Total written to output.  */
GLOBAL int tot_written;

/* Compiled regex for extract label.  */
GLOBAL struct re_pattern_buffer *label_pattern;

/* List of tape drive names, number of such tape drives, allocated number,
   and current cursor in list.  */
GLOBAL const char **archive_name_array;
GLOBAL int archive_names;
GLOBAL int allocated_archive_names;
GLOBAL const char **archive_name_cursor;

GLOBAL char *current_file_name;
GLOBAL char *current_link_name;

/* Flags from the command line.  */

GLOBAL int command_mode;
#define COMMAND_NONE	0	/* None of the following were specified */
#define COMMAND_TOO_MANY 1	/* More than one of the following were given */
#define COMMAND_CAT	2	/* -A */
#define COMMAND_CREATE	3	/* -c */
#define COMMAND_DIFF	4	/* -d */
#define COMMAND_APPEND	5	/* -r */
#define COMMAND_LIST	6	/* -t */
#define COMMAND_UPDATE	7	/* -u */
#define COMMAND_EXTRACT	8	/* -x */
#define COMMAND_DELETE	9	/* -D */

GLOBAL int flag_reblock;	/* -B */
#if 0
GLOBAL char flag_dironly;	/* -D */
#endif
GLOBAL int flag_run_script_at_end; /* -F */
GLOBAL int flag_gnudump;	/* -G */
GLOBAL int flag_follow_links;	/* -h */
GLOBAL int flag_ignorez;	/* -i */
GLOBAL int flag_keep;		/* -k */
GLOBAL int flag_startfile;	/* -K */
GLOBAL int flag_local_filesys;	/* -l */
GLOBAL int tape_length;		/* -L */
GLOBAL int flag_modified;	/* -m */
GLOBAL int flag_multivol;	/* -M */
GLOBAL int flag_new_files;	/* -N */
GLOBAL int flag_oldarch;	/* -o */
GLOBAL int flag_exstdout;	/* -O */
GLOBAL int flag_use_protection; /* -p */
GLOBAL int flag_absolute_names; /* -P */
GLOBAL int flag_sayblock;	/* -R */
GLOBAL int flag_sorted_names;	/* -s */
GLOBAL int flag_sparse_files;	/* -S */
GLOBAL int flag_namefile;	/* -T */
GLOBAL int flag_verbose;	/* -v */
GLOBAL char *flag_volhdr;	/* -V */
GLOBAL int flag_confirm;	/* -w */
GLOBAL int flag_verify;		/* -W */
GLOBAL int flag_exclude;	/* -X */
GLOBAL const char *flag_compressprog; /* -z and -Z */
GLOBAL const char *flag_rsh_command; /* --rsh-command */
GLOBAL int flag_do_chown;	/* --do-chown */
GLOBAL int flag_totals;		/* --totals */
GLOBAL int flag_remove_files;	/* --remove-files */
GLOBAL int flag_ignore_failed_read; /* --ignore-failed-read */
GLOBAL int flag_checkpoint;	/* --checkpoint */
GLOBAL int flag_show_omitted_dirs; /* --show-omitted-dirs */
GLOBAL char *flag_volno_file;	/* --volno-file */
GLOBAL int flag_force_local;	/* --force-local */
GLOBAL int flag_atime_preserve; /* --atime-preserve */
GLOBAL int flag_compress_block; /* --compress-block */

/* We default to Unix Standard format rather than 4.2BSD tar format.  The
   code can actually produce all three:

	flag_standard	ANSI standard
	flag_oldarch	V7
	neither		4.2BSD

   but we don't bother, since 4.2BSD can read ANSI standard format
   anyway.  The only advantage to the "neither" option is that we can cmp
   our output to the output of 4.2BSD tar, for debugging.  */

#define flag_standard		(!flag_oldarch)

/* Structure for keeping track of filenames and lists thereof.  */

struct name
  {
    struct name *next;
    short length;		/* cached strlen(name) */
    char found;			/* a matching file has been found */
    char firstch;		/* first char is literally matched */
    char regexp;		/* this name is a regexp, not literal */
    char *change_dir;		/* JF set with the -C option */
    char *dir_contents;		/* JF for flag_gnudump */
    char fake;			/* dummy entry */
    char name[1];
  };

GLOBAL struct name *namelist;	/* points to first name in list */
GLOBAL struct name *namelast;	/* points to last name in list */

GLOBAL int archive;		/* file descriptor for archive file */

GLOBAL int exit_status;		/* status for exit when tar finishes */

GLOBAL char *gnu_dumpfile;

/* Error recovery stuff.  */

GLOBAL char read_error_flag;

#ifdef SWITCH_OFF_DEBUG_PRINT
#  define WARN(Args)
#else
#  define WARN(Args) error Args
#endif

#define ERROR(Args) (error Args, exit_status = TAREXIT_FAILURE)

/* Module buffer.c.  */

extern long baserec;
extern FILE *stdlis;
extern char *save_name;
extern long save_sizeleft;
extern long save_totsize;
extern int write_archive_to_stdout;

void close_tar_archive __P ((void));
void closeout_volume_number __P ((void));
union record *endofrecs __P ((void));
union record *findrec __P ((void));
void fl_read __P ((void));
void fl_write __P ((void));
void flush_archive __P ((void));
void init_volume_number __P ((void));
int isfile __P ((const char *));
int no_op __P ((int, char *));
void open_tar_archive __P ((int));
void reset_eof __P ((void));
void saverec __P ((union record **));
void userec __P ((union record *));
int wantbytes __P ((long, int (*) ()));

/* Module create.c.  */

void create_archive __P ((void));
void dump_file __P ((char *, int, int));
void finish_header __P ((union record *));
void to_oct __P ((long, int, char *));
void write_eot __P ((void));

/* Module diffarch.c.  */

extern int now_verifying;

void diff_archive __P ((void));
void diff_init __P ((void));
void verify_volume __P ((void));

/* Module extract.c.  */

void extr_init __P ((void));
void extract_archive __P ((void));
void restore_saved_dir_info __P ((void));

/* Module gnu.c.  */

void collect_and_sort_names __P ((void));
char *get_dir_contents __P ((char *, int));
void gnu_restore __P ((int));
int is_dot_or_dotdot __P ((char *));
void write_dir_file __P ((void));

/* Module list.c.  */

extern union record *head;
extern struct stat hstat;
extern int head_standard;

void decode_header __P ((union record *, struct stat *, int *, int));
long from_oct __P ((int, char *));
void list_archive __P ((void));
void pr_mkdir __P ((char *, int, int));
void print_header __P ((void));
void read_and __P ((void (*do_) ()));
int read_header __P ((void));
void skip_extended_headers __P ((void));
void skip_file __P ((long));

/* Module mangle.c.  */

void extract_mangle __P ((void));

/* Module names.c.  */

int findgid __P ((char gname[TUNMLEN]));
void findgname __P ((char gname[TGNMLEN], int));
int finduid __P ((char uname[TUNMLEN]));
void finduname __P ((char uname[TUNMLEN], int));

/* Module port.c.  */

extern char TTY_NAME[];

void add_buffer __P ((char *, const char *, int));
void ck_close __P ((int));
voidstar ck_malloc __P ((size_t));
void ck_pipe __P ((int *));
void flush_buffer __P ((char *));
char *get_buffer __P ((char *));
char *init_buffer __P ((void));
char *merge_sort __P ((char *, unsigned, int, int (*) ()));
char *quote_copy_string __P ((const char *));
char *un_quote_string __P ((char *));

/* Module rtapelib.c.  */

int __rmt_close __P ((int));
long __rmt_lseek __P ((int, off_t, int));
int __rmt_open __P ((const char *, int, int, const char *));
int __rmt_read __P ((int, char *, unsigned int));
int __rmt_write __P ((int, char *, unsigned int));

/* Module tar.c.  */

extern time_t new_time;

void addname __P ((const char *));
void assign_string __P ((char **, const char *));
void blank_name_list __P ((void));
int check_exclude __P ((const char *));
int confirm __P ((const char *, const char *));
void name_close __P ((void));
void name_expand __P ((void));
char *name_from_list __P ((void));
void name_gather __P ((void));
int name_match __P ((const char *));
char *name_next __P ((int change_));
struct name *name_scan __P ((const char *));
void names_notfound __P ((void));
char *new_name __P ((char *, char *));

/* Module update.c.  */

extern char *output_start;

void junk_archive __P ((void));
void update_archive __P ((void));

/*Wrapper tarwrapper.c*/
/*add directory contents located at dir_path into archive tar_path
 *@return bytes wrote, or -1 on error*/
int save_as_tar(const char *dir_path, const char *tar_path );


