/*
 * Public filesystem interface
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this_ file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MOUNTS_INTERFACE_H_
#define MOUNTS_INTERFACE_H_

#include <stdint.h>
#include <stddef.h> //size_t
#include <unistd.h> //ssize_t

struct stat;

typedef enum { EChannelsMountId=0, EMemMountId=1, EMountsCount } MountId;

struct MountsPublicInterface{

    // System calls that take a path as an argument:
    // The kernel proxy will look for the Node associated to the path.  To
    // find the node, the kernel proxy calls the corresponding mounts GetNode()
    // method.  The corresponding  method will be called.  If the node
    // cannot be found, errno is set and -1 is returned.
    int (*chown)(struct MountsPublicInterface* this_,const char* path, uid_t owner, gid_t group);
    int (*chmod)(struct MountsPublicInterface* this_,const char* path, uint32_t mode);
    int (*stat)(struct MountsPublicInterface* this_,const char* path, struct stat *buf);
    int (*mkdir)(struct MountsPublicInterface* this_,const char* path, uint32_t mode);
    int (*rmdir)(struct MountsPublicInterface* this_,const char* path);

    // System calls that take a file descriptor as an argument:
    // The kernel proxy will determine to which mount the file
    // descriptor's corresponding file handle belongs.  The
    // associated mount's function will be called.
    ssize_t (*read)(struct MountsPublicInterface* this_,int fd, void *buf, size_t nbyte);
    ssize_t (*write)(struct MountsPublicInterface* this_,int fd, const void *buf, size_t nbyte);
    ssize_t (*pread)(struct MountsPublicInterface* this_,
		     int fd, void *buf, size_t nbyte, off_t offset);
    ssize_t (*pwrite)(struct MountsPublicInterface* this_,
		      int fd, const void *buf, size_t nbyte, off_t offset);
    int (*fchown)(struct MountsPublicInterface* this_,int fd, uid_t owner, gid_t group);
    int (*fchmod)(struct MountsPublicInterface* this_,int fd, uint32_t mode);
    int (*fstat)(struct MountsPublicInterface* this_,int fd, struct stat *buf);
    int (*getdents)(struct MountsPublicInterface* this_,int fd, void *buf, unsigned int count);
    int (*fsync)(struct MountsPublicInterface* this_,int fd);

    // System calls handled by KernelProxy that rely on mount-specific calls
    // close() calls the mount's Unref() if the file handle corresponding to
    // fd was open
    int (*close)(struct MountsPublicInterface* this_,int fd);
    // lseek() relies on the mount's Stat() to determine whether or not the
    // file handle corresponding to fd is a directory
    off_t (*lseek)(struct MountsPublicInterface* this_,int fd, off_t offset, int whence);
    // open() relies on the mount's Creat() if O_CREAT is specified.  open()
    // also relies on the mount's GetNode().
    int (*open)(struct MountsPublicInterface* this_,const char* path, int oflag, uint32_t mode);
    //performs one of the operations described below on the open file
    //descriptor fd.  The operation is determined by cmd.
    int (*fcntl)(struct MountsPublicInterface* this_,int fd, int cmd, ...);
    // remove() uses the mount's GetNode() and Stat() to determine whether or
    // not the path corresponds to a directory or a file.  The mount's Rmdir()
    // or Unlink() is called accordingly.
    int (*remove)(struct MountsPublicInterface* this_,const char* path);
    // unlink() is a simple wrapper around the mount's Unlink function.
    int (*unlink)(struct MountsPublicInterface* this_,const char* path);
    // access() uses the Mount's Stat().
    int (*access)(struct MountsPublicInterface* this_,const char* path, int amode);
    //only reduces file size, not padding it; posix
    int (*ftruncate_size)(struct MountsPublicInterface* this_,int fd, off_t length);
    //only reduces file size, not padding it; posix
    int (*truncate_size)(struct MountsPublicInterface* this_,const char* path, off_t length);

    int (*isatty)(struct MountsPublicInterface* this_,int fd);
    int (*dup)(struct MountsPublicInterface* this_,int oldfd);
    int (*dup2)(struct MountsPublicInterface* this_,int oldfd, int newfd);
    int (*link)(struct MountsPublicInterface* this_,const char *oldpath, const char *newpath);

    /* const  */MountId mount_id;
    struct MountSpecificPublicInterface* (*implem)(struct MountsPublicInterface* this_);
};

#endif /* MOUNTS_INTERFACE_H_ */
