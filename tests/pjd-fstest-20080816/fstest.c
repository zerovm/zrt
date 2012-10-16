/*-
 * Copyright (c) 2006-2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/tools/regression/fstest/fstest.c,v 1.1 2007/01/17 01:42:07 pjd Exp $
 */

#ifdef linux
#define _GNU_SOURCE
#endif
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <grp.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "zrt.h"
#include "zrtlog.h"

/*if that data reading from stding application should exit*/
#define CONTROL_DATA "test12345complete"
#define PARAM_STRING_MAXSIZE 10240
#define DEBUG
#define PARSING_DEBUG

#ifndef HAS_TRUNCATE64
#define	truncate64	truncate
#endif
#ifndef HAS_STAT64
#define	stat64	stat
#define	lstat64	lstat
#endif

#ifndef ALLPERMS
#define	ALLPERMS	(S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)
#endif

enum action {
    ACTION_OPEN,
    ACTION_CREATE,
    ACTION_UNLINK,
    ACTION_MKDIR,
    ACTION_RMDIR,
    ACTION_LINK,
    ACTION_SYMLINK,
    ACTION_RENAME,
    ACTION_MKFIFO,
    ACTION_CHMOD,
#ifdef HAS_LCHMOD
    ACTION_LCHMOD,
#endif
    ACTION_CHOWN,
    ACTION_LCHOWN,
#ifdef HAS_CHFLAGS
    ACTION_CHFLAGS,
#endif
#ifdef HAS_LCHFLAGS
    ACTION_LCHFLAGS,
#endif
    ACTION_TRUNCATE,
    ACTION_STAT,
    ACTION_LSTAT,
};

#define	TYPE_NONE	0x0000
#define	TYPE_STRING	0x0001
#define	TYPE_NUMBER	0x0002

#define	TYPE_OPTIONAL	0x0100

#define	MAX_ARGS	8

struct syscall_desc {
    char		*sd_name;
    enum action	 sd_action;
    int		 sd_args[MAX_ARGS];
};

static s_counter=0;

static struct syscall_desc syscalls[] = {
        { "open", ACTION_OPEN, { TYPE_STRING, TYPE_STRING, TYPE_NUMBER | TYPE_OPTIONAL, TYPE_NONE } },
        { "create", ACTION_CREATE, { TYPE_STRING, TYPE_NUMBER, TYPE_NONE } },
        { "unlink", ACTION_UNLINK, { TYPE_STRING, TYPE_NONE } },
        { "mkdir", ACTION_MKDIR, { TYPE_STRING, TYPE_NUMBER, TYPE_NONE } },
        { "rmdir", ACTION_RMDIR, { TYPE_STRING, TYPE_NONE } },
        { "link", ACTION_LINK, { TYPE_STRING, TYPE_STRING, TYPE_NONE } },
        { "symlink", ACTION_SYMLINK, { TYPE_STRING, TYPE_STRING, TYPE_NONE } },
        { "rename", ACTION_RENAME, { TYPE_STRING, TYPE_STRING, TYPE_NONE } },
        { "mkfifo", ACTION_MKFIFO, { TYPE_STRING, TYPE_NUMBER, TYPE_NONE } },
        { "chmod", ACTION_CHMOD, { TYPE_STRING, TYPE_NUMBER, TYPE_NONE } },
#ifdef HAS_LCHMOD
        { "lchmod", ACTION_LCHMOD, { TYPE_STRING, TYPE_NUMBER, TYPE_NONE } },
#endif
        { "chown", ACTION_CHOWN, { TYPE_STRING, TYPE_NUMBER, TYPE_NUMBER, TYPE_NONE } },
        { "lchown", ACTION_LCHOWN, { TYPE_STRING, TYPE_NUMBER, TYPE_NUMBER, TYPE_NONE } },
#ifdef HAS_CHFLAGS
        { "chflags", ACTION_CHFLAGS, { TYPE_STRING, TYPE_STRING, TYPE_NONE } },
#endif
#ifdef HAS_LCHFLAGS
        { "lchflags", ACTION_LCHFLAGS, { TYPE_STRING, TYPE_STRING, TYPE_NONE } },
#endif
        { "truncate", ACTION_TRUNCATE, { TYPE_STRING, TYPE_NUMBER, TYPE_NONE } },
        { "stat", ACTION_STAT, { TYPE_STRING, TYPE_STRING, TYPE_NONE } },
        { "lstat", ACTION_LSTAT, { TYPE_STRING, TYPE_STRING, TYPE_NONE } },
        { NULL, -1, { TYPE_NONE } }
};

struct flag {
    long long	 f_flag;
    char		*f_str;
};

static struct flag open_flags[] = {
#ifdef O_RDONLY
        { O_RDONLY, "O_RDONLY" },
#endif
#ifdef O_WRONLY
        { O_WRONLY, "O_WRONLY" },
#endif
#ifdef O_RDWR
        { O_RDWR, "O_RDWR" },
#endif
#ifdef O_NONBLOCK
        { O_NONBLOCK, "O_NONBLOCK" },
#endif
#ifdef O_APPEND
        { O_APPEND, "O_APPEND" },
#endif
#ifdef O_CREAT
        { O_CREAT, "O_CREAT" },
#endif
#ifdef O_TRUNC
        { O_TRUNC, "O_TRUNC" },
#endif
#ifdef O_EXCL
        { O_EXCL, "O_EXCL" },
#endif
#ifdef O_SHLOCK
        { O_SHLOCK, "O_SHLOCK" },
#endif
#ifdef O_EXLOCK
        { O_EXLOCK, "O_EXLOCK" },
#endif
#ifdef O_DIRECT
        { O_DIRECT, "O_DIRECT" },
#endif
#ifdef O_FSYNC
        { O_FSYNC, "O_FSYNC" },
#endif
#ifdef O_SYNC
        { O_SYNC, "O_SYNC" },
#endif
#ifdef O_NOFOLLOW
        { O_NOFOLLOW, "O_NOFOLLOW" },
#endif
#ifdef O_NOCTTY
        { O_NOCTTY, "O_NOCTTY" },
#endif
        { 0, NULL }
};

#ifdef HAS_CHFLAGS
static struct flag chflags_flags[] = {
#ifdef UF_NODUMP
        { UF_NODUMP, "UF_NODUMP" },
#endif
#ifdef UF_IMMUTABLE
        { UF_IMMUTABLE, "UF_IMMUTABLE" },
#endif
#ifdef UF_APPEND
        { UF_APPEND, "UF_APPEND" },
#endif
#ifdef UF_NOUNLINK
        { UF_NOUNLINK, "UF_NOUNLINK" },
#endif
#ifdef UF_OPAQUE
        { UF_OPAQUE, "UF_OPAQUE" },
#endif
#ifdef SF_ARCHIVED
        { SF_ARCHIVED, "SF_ARCHIVED" },
#endif
#ifdef SF_IMMUTABLE
        { SF_IMMUTABLE, "SF_IMMUTABLE" },
#endif
#ifdef SF_APPEND
        { SF_APPEND, "SF_APPEND" },
#endif
#ifdef SF_NOUNLINK
        { SF_NOUNLINK, "SF_NOUNLINK" },
#endif
#ifdef SF_SNAPSHOT
        { SF_SNAPSHOT, "SF_SNAPSHOT" },
#endif
        { 0, NULL }
};
#endif

static const char *err2str(int error);

static void
usage(void)
{

    fprintf(stderr, "usage: fstest [-u uid] [-g gid1[,gid2[...]]] syscall args ...\n");
    return;
}

static long long
str2flags(struct flag *tflags, char *sflags)
{
    long long flags = 0;
    unsigned int i;
    char *f;

    for (f = strtok(sflags, ","); f != NULL; f = strtok(NULL, ",")) {
        /* Support magic 'none' flag which just reset all flags. */
        if (strcmp(f, "none") == 0)
            return (0);
        for (i = 0; tflags[i].f_str != NULL; i++) {
            if (strcmp(tflags[i].f_str, f) == 0)
                break;
        }
        if (tflags[i].f_str == NULL) {
            fprintf(stderr, "unknown flag '%s'\n", f);
            exit(1);
        }
        flags |= tflags[i].f_flag;
    }
    return (flags);
}

#ifdef HAS_CHFLAGS
static char *
flags2str(struct flag *tflags, long long flags)
{
    static char sflags[1024];
    unsigned int i;

    sflags[0] = '\0';
    for (i = 0; tflags[i].f_str != NULL; i++) {
        if (flags & tflags[i].f_flag) {
            if (sflags[0] != '\0')
                strlcat(sflags, ",", sizeof(sflags));
            strlcat(sflags, tflags[i].f_str, sizeof(sflags));
        }
    }
    if (sflags[0] == '\0')
        strlcpy(sflags, "none", sizeof(sflags));
    return (sflags);
}
#endif

static struct syscall_desc *
find_syscall(const char *name)
{
    int i;

    for (i = 0; syscalls[i].sd_name != NULL; i++) {
        if (strcmp(syscalls[i].sd_name, name) == 0)
            return (&syscalls[i]);
    }
    return (NULL);
}

static void
show_stat(struct stat64 *sp, const char *what)
{

    if (strcmp(what, "mode") == 0)
        printf("0%o", (unsigned int)(sp->st_mode & ALLPERMS));
    else if (strcmp(what, "inode") == 0)
        printf("%lld", (long long)sp->st_ino);
    else if (strcmp(what, "nlink") == 0)
        printf("%lld", (long long)sp->st_nlink);
    else if (strcmp(what, "uid") == 0)
        printf("%d", (int)sp->st_uid);
    else if (strcmp(what, "gid") == 0)
        printf("%d", (int)sp->st_gid);
    else if (strcmp(what, "size") == 0)
        printf("%lld", (long long)sp->st_size);
    else if (strcmp(what, "blocks") == 0)
        printf("%lld", (long long)sp->st_blocks);
    else if (strcmp(what, "atime") == 0)
        printf("%lld", (long long)sp->st_atime);
    else if (strcmp(what, "mtime") == 0)
        printf("%lld", (long long)sp->st_mtime);
    else if (strcmp(what, "ctime") == 0)
        printf("%lld", (long long)sp->st_ctime);
#ifdef HAS_CHFLAGS
    else if (strcmp(what, "flags") == 0)
        printf("%s", flags2str(chflags_flags, sp->st_flags));
#endif
    else if (strcmp(what, "type") == 0) {
        switch (sp->st_mode & S_IFMT) {
        case S_IFIFO:
            printf("fifo");
            break;
        case S_IFCHR:
            printf("char");
            break;
        case S_IFDIR:
            printf("dir");
            break;
        case S_IFBLK:
            printf("block");
            break;
        case S_IFREG:
            printf("regular");
            break;
        case S_IFLNK:
            printf("symlink");
            break;
        case S_IFSOCK:
            printf("socket");
            break;
        default:
            printf("unknown");
            break;
        }
    } else {
        printf("unknown");
    }
}

static void
show_stats(struct stat64 *sp, char *what)
{
    const char *s = "";
    char *w;

    for (w = strtok(what, ","); w != NULL; w = strtok(NULL, ",")) {
        printf("%s", s);
        show_stat(sp, w);
        s = ",";
    }
    printf("\n");
}

static unsigned int
call_syscall(struct syscall_desc *scall, char *argv[])
{
    struct stat64 sb;
    long long flags;
    unsigned int i;
    char *endp;
    int rval;
    union {
        char *str;
        long long num;
    } args[MAX_ARGS];

    /*
     * Verify correctness of the arguments.
     */
    for (i = 0; i < sizeof(args)/sizeof(args[0]); i++) {
        if (scall->sd_args[i] == TYPE_NONE) {
            if (argv[i] == NULL || strcmp(argv[i], ":") == 0)
                break;
            fprintf(stderr, "too many arguments [%s]\n", argv[i]);
            return -1;
        } else {
            if (argv[i] == NULL || strcmp(argv[i], ":") == 0) {
                if (scall->sd_args[i] & TYPE_OPTIONAL)
                    break;
                fprintf(stderr, "too few arguments\n");
                return -1;
            }
            if (scall->sd_args[i] & TYPE_STRING) {
                if (strcmp(argv[i], "NULL") == 0)
                    args[i].str = NULL;
                else if (strcmp(argv[i], "DEADCODE") == 0)
                    args[i].str = (void *)0xdeadc0de;
                else
                    args[i].str = argv[i];
            } else if (scall->sd_args[i] & TYPE_NUMBER) {
                args[i].num = strtoll(argv[i], &endp, 0);
                if (*endp != '\0' && !isspace((unsigned char)*endp)) {
                    fprintf(stderr, "invalid argument %u, number expected [%s]\n", i, endp);
                    return -1;
                }
            }
        }
    }
    /*
     * Call the given syscall.
     */
#define	NUM(n)	(args[(n)].num)
#define	STR(n)	(args[(n)].str)
    switch (scall->sd_action) {
    case ACTION_OPEN:
        flags = str2flags(open_flags, STR(1));
        if (flags & O_CREAT) {
            if (i == 2) {
                fprintf(stderr, "too few arguments\n");
                return -1;
            }
            rval = open(STR(0), flags, (mode_t)NUM(2));
        } else {
            if (i == 3) {
                fprintf(stderr, "too many arguments\n");
                return -1;
            }
            rval = open(STR(0), flags);
        }
        fprintf(stderr, "rval=%d, errno=%d\n", rval, errno);
        break;
    case ACTION_CREATE:
        rval = open(STR(0), O_CREAT | O_EXCL, NUM(1));
        if (rval >= 0)
            close(rval);
        break;
    case ACTION_UNLINK:
        rval = unlink(STR(0));
        break;
    case ACTION_MKDIR:
        rval = mkdir(STR(0), NUM(1));
        break;
    case ACTION_RMDIR:
        rval = rmdir(STR(0));
        break;
    case ACTION_LINK:
        rval = link(STR(0), STR(1));
        break;
    case ACTION_SYMLINK:
        rval = symlink(STR(0), STR(1));
        break;
    case ACTION_RENAME:
        rval = rename(STR(0), STR(1));
        break;
    case ACTION_MKFIFO:
        rval = mkfifo(STR(0), NUM(1));
        break;
    case ACTION_CHMOD:
        rval = chmod(STR(0), NUM(1));
        break;
#ifdef HAS_LCHMOD
    case ACTION_LCHMOD:
        rval = lchmod(STR(0), NUM(1));
        break;
#endif
    case ACTION_CHOWN:
        rval = chown(STR(0), NUM(1), NUM(2));
        break;
    case ACTION_LCHOWN:
        rval = lchown(STR(0), NUM(1), NUM(2));
        break;
#ifdef HAS_CHFLAGS
    case ACTION_CHFLAGS:
        rval = chflags(STR(0), str2flags(chflags_flags, STR(1)));
        break;
#endif
#ifdef HAS_LCHFLAGS
    case ACTION_LCHFLAGS:
        rval = lchflags(STR(0), str2flags(chflags_flags, STR(1)));
        break;
#endif
    case ACTION_TRUNCATE:
        rval = truncate64(STR(0), NUM(1));
        break;
    case ACTION_STAT:
        rval = stat64(STR(0), &sb);
        if (rval == 0) {
            show_stats(&sb, STR(1));
            return (i);
        }
        break;
    case ACTION_LSTAT:
        rval = lstat64(STR(0), &sb);
        if (rval == 0) {
            show_stats(&sb, STR(1));
            return (i);
        }
        break;
    default:
        fprintf(stderr, "unsupported syscall\n");
        return -1;
    }
#undef STR
#undef NUM
    if (rval < 0) {
        const char *serrno;

        serrno = err2str(errno);
        fprintf(stderr, "%s returned %d\n", scall->sd_name, rval);
        printf("%s\n", serrno);
        return -1;
    }
    fprintf(stderr, "0\n");
    printf("0\n");
    return (i);
}

static int
set_gids(char *gids)
{
    gid_t *gidset;
    long ngroups;
    char *g, *endp;
    unsigned i;

    ngroups = sysconf(_SC_NGROUPS_MAX);
    assert(ngroups > 0);
    gidset = malloc(sizeof(*gidset) * ngroups);
    assert(gidset != NULL);
    for (i = 0, g = strtok(gids, ","); g != NULL; g = strtok(NULL, ","), i++) {
        if (i >= ngroups) {
            fprintf(stderr, "too many gids\n");
            return 1;
        }
        gidset[i] = strtol(g, &endp, 0);
        if (*endp != '\0' && !isspace((unsigned char)*endp)) {
            fprintf(stderr, "invalid gid '%s' - number expected\n",
                    g);
            return 1;
        }
    }
    if (setgroups(i, gidset) < 0) {
        fprintf(stderr, "cannot change groups: %s\n", strerror(errno));
        const char* serrno = err2str(errno);
        printf("%s\n", serrno);
        return 1;
    }
    if (setegid(gidset[0]) < 0) {
        fprintf(stderr, "cannot change effective gid: %s\n", strerror(errno));
        const char* serrno = err2str(errno);
        printf("%s\n", serrno);
        return 1;
    }
    free(gidset);
}


static int
read_stdin_parameters( char* argv[] ){
    const int stdin=0;
    int words_count=0;
    int cursor = 0;
    int condition=0;
    char *lexema=NULL;

    lexema = argv[0];
    /*read string up to $$ trailing chars*/
    char c;
    char prev = 0;
    do{
        prev=c;
        ssize_t bytes = read(stdin, &c, 1);
        if ( bytes <= 0 ){
            continue;
        }

#ifdef PARSING_DEBUG
        fprintf(stderr, "c=%c\n", c);
        fprintf(stderr, "words_count=%d, lexema=%s\n", words_count, lexema);
#endif
        if ( c == ' ' || c == '\n' ){
            /*skip space */
        }
        else{
            if ( prev == ' ' || prev == '\n' ){
                lexema[cursor] = '\0';
                lexema = argv[++words_count];
                cursor=0;
            }
            /*save readed char*/
            lexema[cursor++] = c;
        }

        /*check condition*/
        if ( c == '\n' ){
            condition = 1;
            lexema[cursor] = '\0';
        }
    }while( !condition ); /*condition is 1 only if trailing $$ pair reached*/

    #ifdef PARSING_DEBUG
        fprintf(stderr, "parsed%s ", ":" );
        int i;
        for(i=0; i < words_count; i++){
            fprintf(stderr, "%s ", argv[i] );
        }
        fprintf(stderr, "%s", "\n" );
    #endif

    /*do not forget last word*/
    return ++words_count;
}

int run_syscall(int argc, char *argv[]){
    struct syscall_desc *scall;
    unsigned int n;
    char *gids, *endp;
    int uid, umsk, ch;

    uid = -1;
    gids = NULL;
    umsk = 0;
    ++s_counter;

    /*reset getopt variables*/
    optarg = NULL;
    optind=0;
    optopt=0;

#ifdef DEBUG
    fprintf(stderr, "syscall[%d]:", s_counter );
    int i;
    for(i=0; i < argc; i++){
        fprintf(stderr, "%s ", argv[i] );
    }
    fprintf(stderr, "%s", "\n" );
#endif


    while ((ch = getopt(argc, argv, "g:u:U:")) != -1) {
        switch(ch) {
        case 'g':
            gids = optarg;
            break;
        case 'u':
            uid = (int)strtol(optarg, &endp, 0);
            if (*endp != '\0' && !isspace((unsigned char)*endp)) {
                fprintf(stderr, "invalid uid '%s' - number "
                        "expected\n", optarg);
                return 1;
            }
            break;
        case 'U':
            umsk = (int)strtol(optarg, &endp, 0);
            if (*endp != '\0' && !isspace((unsigned char)*endp)) {
                fprintf(stderr, "invalid umask '%s' - number "
                        "expected\n", optarg);
                return 1;
            }
            break;
        default:
            fprintf( stderr, "unknown option: %c\n", ch);
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1) {
        fprintf(stderr, "too few arguments\n");
        usage();
        return 1;
    }

    if (gids != NULL) {
        fprintf(stderr, "changing groups to %s\n", gids);
        return set_gids(gids);
    }
    if (uid != -1) {
        fprintf(stderr, "changing uid to %d\n", uid);
        errno=ENOSYS;
        if (setuid(uid) < 0) {
            fprintf(stderr, "cannot change uid: %s\n",
                    strerror(errno));
            const char* serrno = err2str(errno);
            printf("%s\n", serrno);
            return 1;
        }
    }

    /* Change umask to requested value or to 0, if not requested. */
    if ( umsk ){
        mode_t prev = umask(umsk);
        fprintf( stderr, "get prev umask=%o, new=%o\n", prev, umsk );
    }

    for (;;) {
        scall = find_syscall(argv[0]);
        if (scall == NULL) {
            fprintf(stderr, "syscall '%s' not supported\n", argv[0]);
            return 1;
        }
        argc++;
        argv++;
        n = call_syscall(scall, argv);
        if ( n == -1 ) return 1;
        argc += n;
        argv += n;
        if (argv[0] == NULL)
            break;
        argc++;
        argv++;
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    /*alloc array of strings it will be used for sequential run of syscalls*/
    const int array_len=10;
    char *stdin_argv[array_len];
    int i;
    for ( i=0; i < array_len; i++ ){
        stdin_argv[i] = malloc(PARAM_STRING_MAXSIZE);
    }
    /*read while no parameters will be returned*/
    int stdin_argc;
    while( (stdin_argc=read_stdin_parameters( (char**)stdin_argv )) ){
        /*should be guarantied that last item is null, since we can't erase allocated pointer stdin_arg[]
         * so we need to copy it to the new array*/
        char *syscall_argv[stdin_argc+1];
        for ( i=0; i < stdin_argc; i++ ){
            syscall_argv[i] = malloc( strlen(stdin_argv[i])+1 );
            strcpy( syscall_argv[i], stdin_argv[i] );

            /*check termination control data*/
            if ( !strcmp(stdin_argv[i], CONTROL_DATA) ){
                fprintf(stderr, "recevied %s\n", "kill");
                exit(0);
            }
        }
        syscall_argv[stdin_argc] = NULL;

        /*emulate pseudo command line data*/
        int ret = run_syscall( stdin_argc, syscall_argv );
#ifdef DEBUG
        fprintf(stderr, "run_syscall complete %d\n", ret);
#endif
        for ( i=0; i < stdin_argc; i++ ){
            free( syscall_argv[i] );
        }
    }

    for ( i=0; i < array_len; i++ ){
        free( stdin_argv[i] );
    }
    exit(0);
}

static const char *
err2str(int error)
{
    static char errnum[8];

    switch (error) {
#ifdef EPERM
    case EPERM:
        return ("EPERM");
#endif
#ifdef ENOENT
    case ENOENT:
        return ("ENOENT");
#endif
#ifdef ESRCH
    case ESRCH:
        return ("ESRCH");
#endif
#ifdef EINTR
    case EINTR:
        return ("EINTR");
#endif
#ifdef EIO
    case EIO:
        return ("EIO");
#endif
#ifdef ENXIO
    case ENXIO:
        return ("ENXIO");
#endif
#ifdef E2BIG
    case E2BIG:
        return ("E2BIG");
#endif
#ifdef ENOEXEC
    case ENOEXEC:
        return ("ENOEXEC");
#endif
#ifdef EBADF
    case EBADF:
        return ("EBADF");
#endif
#ifdef ECHILD
    case ECHILD:
        return ("ECHILD");
#endif
#ifdef EDEADLK
    case EDEADLK:
        return ("EDEADLK");
#endif
#ifdef ENOMEM
    case ENOMEM:
        return ("ENOMEM");
#endif
#ifdef EACCES
    case EACCES:
        return ("EACCES");
#endif
#ifdef EFAULT
    case EFAULT:
        return ("EFAULT");
#endif
#ifdef ENOTBLK
    case ENOTBLK:
        return ("ENOTBLK");
#endif
#ifdef EBUSY
    case EBUSY:
        return ("EBUSY");
#endif
#ifdef EEXIST
    case EEXIST:
        return ("EEXIST");
#endif
#ifdef EXDEV
    case EXDEV:
        return ("EXDEV");
#endif
#ifdef ENODEV
    case ENODEV:
        return ("ENODEV");
#endif
#ifdef ENOTDIR
    case ENOTDIR:
        return ("ENOTDIR");
#endif
#ifdef EISDIR
    case EISDIR:
        return ("EISDIR");
#endif
#ifdef EINVAL
    case EINVAL:
        return ("EINVAL");
#endif
#ifdef ENFILE
    case ENFILE:
        return ("ENFILE");
#endif
#ifdef EMFILE
    case EMFILE:
        return ("EMFILE");
#endif
#ifdef ENOTTY
    case ENOTTY:
        return ("ENOTTY");
#endif
#ifdef ETXTBSY
    case ETXTBSY:
        return ("ETXTBSY");
#endif
#ifdef EFBIG
    case EFBIG:
        return ("EFBIG");
#endif
#ifdef ENOSPC
    case ENOSPC:
        return ("ENOSPC");
#endif
#ifdef ESPIPE
    case ESPIPE:
        return ("ESPIPE");
#endif
#ifdef EROFS
    case EROFS:
        return ("EROFS");
#endif
#ifdef EMLINK
    case EMLINK:
        return ("EMLINK");
#endif
#ifdef EPIPE
    case EPIPE:
        return ("EPIPE");
#endif
#ifdef EDOM
    case EDOM:
        return ("EDOM");
#endif
#ifdef ERANGE
    case ERANGE:
        return ("ERANGE");
#endif
#ifdef EAGAIN
    case EAGAIN:
        return ("EAGAIN");
#endif
#ifdef EINPROGRESS
    case EINPROGRESS:
        return ("EINPROGRESS");
#endif
#ifdef EALREADY
    case EALREADY:
        return ("EALREADY");
#endif
#ifdef ENOTSOCK
    case ENOTSOCK:
        return ("ENOTSOCK");
#endif
#ifdef EDESTADDRREQ
    case EDESTADDRREQ:
        return ("EDESTADDRREQ");
#endif
#ifdef EMSGSIZE
    case EMSGSIZE:
        return ("EMSGSIZE");
#endif
#ifdef EPROTOTYPE
    case EPROTOTYPE:
        return ("EPROTOTYPE");
#endif
#ifdef ENOPROTOOPT
    case ENOPROTOOPT:
        return ("ENOPROTOOPT");
#endif
#ifdef EPROTONOSUPPORT
    case EPROTONOSUPPORT:
        return ("EPROTONOSUPPORT");
#endif
#ifdef ESOCKTNOSUPPORT
    case ESOCKTNOSUPPORT:
        return ("ESOCKTNOSUPPORT");
#endif
#ifdef EOPNOTSUPP
    case EOPNOTSUPP:
        return ("EOPNOTSUPP");
#endif
#ifdef EPFNOSUPPORT
    case EPFNOSUPPORT:
        return ("EPFNOSUPPORT");
#endif
#ifdef EAFNOSUPPORT
    case EAFNOSUPPORT:
        return ("EAFNOSUPPORT");
#endif
#ifdef EADDRINUSE
    case EADDRINUSE:
        return ("EADDRINUSE");
#endif
#ifdef EADDRNOTAVAIL
    case EADDRNOTAVAIL:
        return ("EADDRNOTAVAIL");
#endif
#ifdef ENETDOWN
    case ENETDOWN:
        return ("ENETDOWN");
#endif
#ifdef ENETUNREACH
    case ENETUNREACH:
        return ("ENETUNREACH");
#endif
#ifdef ENETRESET
    case ENETRESET:
        return ("ENETRESET");
#endif
#ifdef ECONNABORTED
    case ECONNABORTED:
        return ("ECONNABORTED");
#endif
#ifdef ECONNRESET
    case ECONNRESET:
        return ("ECONNRESET");
#endif
#ifdef ENOBUFS
    case ENOBUFS:
        return ("ENOBUFS");
#endif
#ifdef EISCONN
    case EISCONN:
        return ("EISCONN");
#endif
#ifdef ENOTCONN
    case ENOTCONN:
        return ("ENOTCONN");
#endif
#ifdef ESHUTDOWN
    case ESHUTDOWN:
        return ("ESHUTDOWN");
#endif
#ifdef ETOOMANYREFS
    case ETOOMANYREFS:
        return ("ETOOMANYREFS");
#endif
#ifdef ETIMEDOUT
    case ETIMEDOUT:
        return ("ETIMEDOUT");
#endif
#ifdef ECONNREFUSED
    case ECONNREFUSED:
        return ("ECONNREFUSED");
#endif
#ifdef ELOOP
    case ELOOP:
        return ("ELOOP");
#endif
#ifdef ENAMETOOLONG
    case ENAMETOOLONG:
        return ("ENAMETOOLONG");
#endif
#ifdef EHOSTDOWN
    case EHOSTDOWN:
        return ("EHOSTDOWN");
#endif
#ifdef EHOSTUNREACH
    case EHOSTUNREACH:
        return ("EHOSTUNREACH");
#endif
#ifdef ENOTEMPTY
    case ENOTEMPTY:
        return ("ENOTEMPTY");
#endif
#ifdef EPROCLIM
    case EPROCLIM:
        return ("EPROCLIM");
#endif
#ifdef EUSERS
    case EUSERS:
        return ("EUSERS");
#endif
#ifdef EDQUOT
    case EDQUOT:
        return ("EDQUOT");
#endif
#ifdef ESTALE
    case ESTALE:
        return ("ESTALE");
#endif
#ifdef EREMOTE
    case EREMOTE:
        return ("EREMOTE");
#endif
#ifdef EBADRPC
    case EBADRPC:
        return ("EBADRPC");
#endif
#ifdef ERPCMISMATCH
    case ERPCMISMATCH:
        return ("ERPCMISMATCH");
#endif
#ifdef EPROGUNAVAIL
    case EPROGUNAVAIL:
        return ("EPROGUNAVAIL");
#endif
#ifdef EPROGMISMATCH
    case EPROGMISMATCH:
        return ("EPROGMISMATCH");
#endif
#ifdef EPROCUNAVAIL
    case EPROCUNAVAIL:
        return ("EPROCUNAVAIL");
#endif
#ifdef ENOLCK
    case ENOLCK:
        return ("ENOLCK");
#endif
#ifdef ENOSYS
    case ENOSYS:
        return ("ENOSYS");
#endif
#ifdef EFTYPE
    case EFTYPE:
        return ("EFTYPE");
#endif
#ifdef EAUTH
    case EAUTH:
        return ("EAUTH");
#endif
#ifdef ENEEDAUTH
    case ENEEDAUTH:
        return ("ENEEDAUTH");
#endif
#ifdef EIDRM
    case EIDRM:
        return ("EIDRM");
#endif
#ifdef ENOMSG
    case ENOMSG:
        return ("ENOMSG");
#endif
#ifdef EOVERFLOW
    case EOVERFLOW:
        return ("EOVERFLOW");
#endif
#ifdef ECANCELED
    case ECANCELED:
        return ("ECANCELED");
#endif
#ifdef EILSEQ
    case EILSEQ:
        return ("EILSEQ");
#endif
#ifdef ENOATTR
    case ENOATTR:
        return ("ENOATTR");
#endif
#ifdef EDOOFUS
    case EDOOFUS:
        return ("EDOOFUS");
#endif
#ifdef EBADMSG
    case EBADMSG:
        return ("EBADMSG");
#endif
#ifdef EMULTIHOP
    case EMULTIHOP:
        return ("EMULTIHOP");
#endif
#ifdef ENOLINK
    case ENOLINK:
        return ("ENOLINK");
#endif
#ifdef EPROTO
    case EPROTO:
        return ("EPROTO");
#endif
    default:
        snprintf(errnum, sizeof(errnum), "%d", error);
        return (errnum);
    }
}
