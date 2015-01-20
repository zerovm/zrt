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

static pthread_t s_foo_semaphore_thread;

static void *foo_thread_semaphore_func(void *a)
{
	(void)a;
    return NULL;
}


/* Initialize semaphore object SEM to VALUE.  If PSHARED then share it
   with other processes.  */
int sem_init (sem_t *sem, int pshared, unsigned int value){
  /*as pthread, semaphore implementation expect that any thread was
	created previously therefore we check it and if not created then
	do it now*/
	if (pth_current == NULL && s_foo_semaphore_thread == NULL ){
		pthread_create(&s_foo_semaphore_thread, NULL, foo_thread_semaphore_func, NULL);
	}

	*sem = malloc(sizeof(pth_sem_t));
	if ( pth_sem_init(*sem) == TRUE ){
		if ( pth_sem_set_value(*sem, value) == FALSE ){
			errno=EINVAL;
			return -1;
		}
	}
	/*pshared not relevant for single process single threaded
	  environment*/
	(void)pshared;
	return 0;
}

/* Free resources associated with semaphore object SEM.  */
int sem_destroy (sem_t *sem){
	if (*sem != NULL){ 
		free(*sem);
		*sem = NULL;
		return 0;
	}
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
	if ( pth_sem_dec(*sem) == TRUE )
		return 0;
	else
		return -1;
}

/* Similar to `sem_wait' but wait only until ABSTIME.*/
int sem_timedwait (sem_t *sem, const struct timespec *abstime){
	unsigned int value;
	if ( pth_sem_get_value(*sem, &value) == TRUE ){
		if (value>0){
			return pth_sem_dec(*sem) == TRUE? 0 : -1;
		}
		else{
			struct timespec rem_unused;
			nanosleep(abstime, &rem_unused);
			if ( pth_sem_get_value(*sem, &value) == TRUE ){
				if (value>0){
					return pth_sem_dec(*sem) == TRUE? 0 : -1;
				}
				else{
					errno=ETIMEDOUT;
					return -1;
				}
			}
		}
	}
	return 0;
}

/* Test whether SEM is posted.  */
int sem_trywait (sem_t *sem){
	int ret;
	unsigned value;
	if ( pth_sem_get_value(*sem, &value) == TRUE ){
		/*if can be decremented then do it*/
		if (value>0){
			return pth_sem_dec(*sem) == TRUE? 0 : -1;
		}
		else{
			errno=EAGAIN;
			return -1;
		}
	}
	return 0;
}

/* Post SEM.  */
int sem_post (sem_t *sem){
	if ( pth_sem_inc(*sem, 1) == TRUE )
		return 0;
	else
		return -1;
}

/* Get current value of SEM and store it in *SVAL.  */
int sem_getvalue (sem_t *sem, int *sval){
	unsigned int value;
	if ( pth_sem_get_value(*sem, &value) == TRUE ){
		*sval = value;
		return 0;
	}
	else return -1;
}

