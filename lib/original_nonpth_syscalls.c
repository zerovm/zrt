#include "original_nonpth_syscalls.h"

/*Following functions are sinonymus of syscalls which not redefined
  by pth*/

extern unsigned int __sleep(unsigned int seconds);
extern int __nanosleep(const struct timespec *req, struct timespec *rem);
extern ssize_t __read(int fd, void *buf, size_t count);
extern ssize_t __write(int fd, const void *buf, size_t count);
extern int __select(int nfds, fd_set *readfds, fd_set *writefds,
		    fd_set *exceptfds, struct timeval *timeout);
extern ssize_t __libc_pread(int fd, void *buf, size_t count, off_t offset);
extern ssize_t __libc_pwrite(int fd, const void *buf, size_t count, off_t offset);

unsigned int syscall_sleep(unsigned int seconds){
    return __sleep(seconds);
}

int syscall_nanosleep(const struct timespec *req, struct timespec *rem){
    return __nanosleep(req, rem);
}

ssize_t syscall_read(int fd, void *buf, size_t count){
    return __read(fd, buf, count);
}

ssize_t syscall_write(int fd, const void *buf, size_t count){
    return __write(fd, buf, count);
}

int syscall_select(int nfds, fd_set *readfds, fd_set *writefds,
		   fd_set *exceptfds, struct timeval *timeout){
    return __select(nfds, readfds, writefds, exceptfds, timeout);
}

ssize_t syscall_pread(int fd, void *buf, size_t count, off_t offset){
    return __libc_pread(fd, buf, count, offset);
}

ssize_t syscall_pwrite(int fd, const void *buf, size_t count, off_t offset){
    return __libc_pwrite(fd, buf, count, offset);
}

