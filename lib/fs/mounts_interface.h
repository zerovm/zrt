/*
 * mounts_interface.h
 *
 *  Created on: 19.09.2012
 *      Author: yaroslav
 */

#ifndef MOUNTS_INTERFACE_H_
#define MOUNTS_INTERFACE_H_

#include <stdint.h>
#include <stddef.h> //size_t
#include <unistd.h> //ssize_t

struct stat;

typedef enum { EChannelsMountId=0, EMemMountId=1, EMountsCount } MountId;

struct MountsInterface{

    // System calls that take a path as an argument:
    // The kernel proxy will look for the Node associated to the path.  To
    // find the node, the kernel proxy calls the corresponding mounts GetNode()
    // method.  The corresponding  method will be called.  If the node
    // cannot be found, errno is set and -1 is returned.
    int (*chown)(const char* path, uid_t owner, gid_t group);
    int (*chmod)(const char* path, uint32_t mode);
    int (*stat)(const char* path, struct stat *buf);
    int (*mkdir)(const char* path, uint32_t mode);
    int (*rmdir)(const char* path);
    int (*umount)(const char* path);
    int (*mount)(const char* path, void *mount);

    // System calls that take a file descriptor as an argument:
    // The kernel proxy will determine to which mount the file
    // descriptor's corresponding file handle belongs.  The
    // associated mount's function will be called.
    ssize_t (*read)(int fd, void *buf, size_t nbyte);
    ssize_t (*write)(int fd, const void *buf, size_t nbyte);
    int (*fchown)(int fd, uid_t owner, gid_t group);
    int (*fchmod)(int fd, uint32_t mode);
    int (*fstat)(int fd, struct stat *buf);
    int (*getdents)(int fd, void *buf, unsigned int count);
    int (*fsync)(int fd);

    // System calls handled by KernelProxy that rely on mount-specific calls
    // close() calls the mount's Unref() if the file handle corresponding to
    // fd was open
    int (*close)(int fd);
    // lseek() relies on the mount's Stat() to determine whether or not the
    // file handle corresponding to fd is a directory
    off_t (*lseek)(int fd, off_t offset, int whence);
    // open() relies on the mount's Creat() if O_CREAT is specified.  open()
    // also relies on the mount's GetNode().
    int (*open)(const char* path, int oflag, uint32_t mode);
    //performs one of the operations described below on the open file
    //descriptor fd.  The operation is determined by cmd.
    int (*fcntl)(int fd, int cmd, ...);
    // remove() uses the mount's GetNode() and Stat() to determine whether or
    // not the path corresponds to a directory or a file.  The mount's Rmdir()
    // or Unlink() is called accordingly.
    int (*remove)(const char* path);
    // unlink() is a simple wrapper around the mount's Unlink function.
    int (*unlink)(const char* path);
    // access() uses the Mount's Stat().
    int (*access)(const char* path, int amode);
    //only reduces file size, not padding it; posix
    int (*ftruncate_size)(int fd, off_t length);
    //only reduces file size, not padding it; posix
    int (*truncate_size)(const char* path, off_t length);

    int (*isatty)(int fd);
    int (*dup)(int oldfd);
    int (*dup2)(int oldfd, int newfd);
    int (*link)(const char* path1, const char* path2);

    const MountId mount_id;
    struct mount_specific_implem* (*implem)();
};

#endif /* MOUNTS_INTERFACE_H_ */
