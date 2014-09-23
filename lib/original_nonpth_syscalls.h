#ifndef __PTH_ZRT_H__
#define __PTH_ZRT_H__

#include <time.h> //sleep, nanosleep
#include <unistd.h> //read, write
#include <sys/select.h>

unsigned int syscall_sleep(unsigned int seconds);

int syscall_nanosleep(const struct timespec *req, struct timespec *rem);

ssize_t syscall_read(int fd, void *buf, size_t count);

ssize_t syscall_write(int fd, const void *buf, size_t count);

int syscall_select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout);

ssize_t syscall_pread(int fd, void *buf, size_t count, off_t offset);

ssize_t syscall_pwrite(int fd, const void *buf, size_t count, off_t offset);

#endif //__PTH_ZRT_H__

