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
**  semaphore.c: POSIX Thread ("Pthread") API for Pth
*/

/*
 * Include our own Pthread and then the private Pth header.
 * The order here is very important to get correct namespace hiding!
 */

#include "semaphore.h"
#include "pth_p.h"


/* Initialize semaphore object SEM to VALUE.  If PSHARED then share it
   with other processes.  */
int sem_init (sem_t *sem, int pshared, unsigned int value){
	int ret = pth_sem_init(sem);
	if (ret==0)
		ret = pth_sem_set_value(sem, value);
	/*pshared not relevant for single process single threaded
	  environment*/
	(void)pshared;
	return ret;
}

/* Free resources associated with semaphore object SEM.  */
int sem_destroy (sem_t *sem){
	if (sem != NULL) return 0;
	else{
		errno = EINVAL;
		return -1;
	}
}

#ifdef NAMED_SEMAPHORE_ENABLE
/* Open a named semaphore NAME with open flags OFLAG.  */
sem_t *sem_open (const char *name, int oflag, ...){
	(void)name;
	(void)oflag;
	errno=ENOSYS;
	return NULL;
}

/* Close descriptor for named semaphore SEM.  */
int sem_close (sem_t *sem){
	(void)sem;
	errno=ENOSYS;
	return -1;
}

/* Remove named semaphore NAME.  */
int sem_unlink (const char *name){
	(void)name;
	errno=ENOSYS;
	return -1;
}
#endif


/* Wait for SEM being posted.*/
int sem_wait (sem_t *sem){
	return pth_sem_dec(sem);
}

#ifdef __USE_XOPEN2K
/* Similar to `sem_wait' but wait only until ABSTIME.*/
int sem_timedwait (sem_t *sem, const struct timespec *abstime){
	errno=ENOSYS;
	return -1;
}
#endif

/* Test whether SEM is posted.  */
int sem_trywait (sem_t *sem){
	int ret;
	unsigned val;
	ret = pth_sem_get_value(sem, &val);
	if (ret==0){
			/*if can be decremented then do it*/
		if (val>0){
			return pth_sem_dec(sem);
		}
		ret=-1;
		errno=EAGAIN;
	}
	return ret;
}

/* Post SEM.  */
int sem_post (sem_t *sem){
	return pth_sem_inc(sem, 1);
}

/* Get current value of SEM and store it in *SVAL.  */
int sem_getvalue (sem_t *sem, int *sval){
	unsigned value;
	int ret = pth_sem_get_value(sem, value);
	if (ret==0)
		*sval = value;
	return ret;
}

