/*
**  GNU Pth - The GNU Portable Threads
**  Copyright (c) 2015 Yaroslav Litvinov <yaroslav.litvinov@rackspace.com>
**
**  This file is part of GNU Pth, a non-preemptive thread scheduling
**  library which can be found at http://www.gnu.org/software/pth/.
**
**  This library is free software; you can redistribute it and/or
**  modify it under the terms of the GNU Lesser General Public
**  License as published by the Free Software Foundation; either
**  version 2.1 of the License, or (at your option) any later version.
**
**  This library is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**  Lesser General Public License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License along with this library; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
**  USA, or contact Ralf S. Engelschall <rse@engelschall.com>.
**
**  semaphore.h: Semaphores API for Pth
*/

#ifndef _PTH_SEMAPHORE_H_
#define _PTH_SEMAPHORE_H_

/*
 * Prevent system includes from implicitly including
 * possibly existing vendor Pthread headers
 */
#define _SEMAPHORE_H

/* Value returned if `sem_open' failed.  */
#define SEM_FAILED      ((pth_sem_t *) 0)

/*
Macro NAMED_SEMAPHORE_ENABLE
to enable stubbed implementation of named semaphores*/

struct timespec;

typedef pth_sem_t sem_t;

/*
**
** API DEFINITION
**
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize semaphore object SEM to VALUE.  If PSHARED then share it
   with other processes.  */
extern int sem_init (sem_t *sem, int pshared, unsigned int value);
/* Free resources associated with semaphore object SEM.  */
extern int sem_destroy (sem_t *sem);

#ifdef NAMED_SEMAPHORE_ENABLE
/* Open a named semaphore NAME with open flags OFLAG.  */
extern sem_t *sem_open (const char *name, int oflag, ...);
/* Close descriptor for named semaphore SEM.  */
extern int sem_close (sem_t *sem);
/* Remove named semaphore NAME.  */
extern int sem_unlink (const char *name);
#endif 

/* Wait for SEM being posted.*/
extern int sem_wait (sem_t *sem);

#ifdef __USE_XOPEN2K
/* Similar to `sem_wait' but wait only until ABSTIME.*/
extern int sem_timedwait (sem_t *sem, const struct timespec *abstime);
#endif

/* Test whether SEM is posted.  */
extern int sem_trywait (sem_t *sem);

/* Post SEM.  */
extern int sem_post (sem_t *sem);

/* Get current value of SEM and store it in *SVAL.  */
extern int sem_getvalue (sem_t *sem, int *sval);

#ifdef __cplusplus
}
#endif

#endif /* _PTH_SEMAPHORE_H_ */

