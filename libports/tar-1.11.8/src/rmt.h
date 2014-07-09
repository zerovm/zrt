/* Definitions for communicating with a remote tape drive.
   Copyright (C) 1988, 1992 Free Software Foundation, Inc.

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

extern char *__rmt_path;

int __tar_rmt_open __P ((const char *, int, int, const char *));
int __tar_rmt_close __P ((int));
int __tar_rmt_read __P ((int, char *, unsigned int));
int __tar_rmt_write __P ((int, char *, unsigned int));
long __rmt_lseek __P ((int, off_t, int));
int __tar_rmt_ioctl __P ((int, int, char *));

/* A filename is remote if it contains a colon not preceeded by a slash,
   to take care of `/:/' which is a shorthand for `/.../<CELL-NAME>/fs'
   on machines running OSF's Distributing Computing Environment (DCE) and
   Distributed File System (DFS).  However, when --force-local, a
   filename is never remote.
   
   [Which system?  What is DCE?  What is DFS?]  */

#define _remdev(Path) \
  (!flag_force_local && (__rmt_path = strchr (Path, ':')) \
   && __rmt_path > (Path) && __rmt_path[-1] != '/')

#define _isrmt(Fd) \
  ((Fd) >= __REM_BIAS)

#define __REM_BIAS	128

#ifndef O_CREAT
#define O_CREAT	01000
#endif

#define rmtopen(Path, Oflag, Mode, Command) \
  (_remdev (Path) ? __tar_rmt_open (Path, Oflag, __REM_BIAS, Command) \
   : open (Path, Oflag, Mode))

#define rmtaccess(Path, Amode) \
  (_remdev (Path) ? 0 : access (Path, Amode))

#define rmtstat(Path, Buffer) \
  (_remdev (Path) ? (errno = EOPNOTSUPP), -1 : stat (Path, Buffer))

#define rmtcreat(Path, Mode, Command) \
   (_remdev (Path) \
    ? __tar_rmt_open (Path, 1 | O_CREAT, __REM_BIAS, Command) \
    : creat (Path, Mode))

#define rmtlstat(Path, Buffer) \
  (_remdev (Path) ? (errno = EOPNOTSUPP), -1 : lstat (Path, Buffer))

#define rmtread(Fd, Buffer, Length) \
  (_isrmt (Fd) ? __tar_rmt_read (Fd - __REM_BIAS, Buffer, Length) \
   : read (Fd, Buffer, Length))

#define rmtwrite(Fd, Buffer, Length) \
  (_isrmt (Fd) ? __tar_rmt_write (Fd - __REM_BIAS, Buffer, Length) \
   : write (Fd, Buffer, Length))

#define rmtlseek(Fd, Offset, Where) \
  (_isrmt (Fd) ? __tar_rmt_lseek (Fd - __REM_BIAS, Offset, Where) \
   : lseek (Fd, Offset, Where))

#define rmtclose(Fd) \
  (_isrmt (Fd) ? __tar_rmt_close (Fd - __REM_BIAS) : close (Fd))

#define rmtioctl(Fd, Request, Argument) \
  (_isrmt (Fd) ? __tar_rmt_ioctl (Fd - __REM_BIAS, Request, Argument) \
   : ioctl (Fd, Request, Argument))

#define rmtdup(Fd) \
  (_isrmt (Fd) ? (errno = EOPNOTSUPP), -1 : dup (Fd))

#define rmtfstat(Fd, Buffer) \
  (_isrmt (Fd) ? (errno = EOPNOTSUPP), -1 : fstat (Fd, Buffer))

#define rmtfcntl(Fd, Command, Argument) \
  (_isrmt (Fd) ? (errno = EOPNOTSUPP), -1 : fcntl (Fd, Command, Argument))

#define rmtisatty(Fd) \
  (_isrmt (Fd) ? 0 : isatty (Fd))
