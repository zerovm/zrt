/*
 * ZeroVM runtime
 *
 *  Created on: Jul 3, 2012
 *      Author: d'b, YaroslavLitvinov
 */

#ifndef _LIB_ZRT_H_
#define _LIB_ZRT_H_

#include <stdint.h>

/*
 * user program entry point. should be explicitly defined by user 
 * application instead of main.
 */
int zmain(int argc, char **argv);

/* entry point into untrasted syscalls implementation. It using syscallback
 * mechanism that allows to handle syscalls implemented on untrasted side, 
 * described by zrt_syscalls array in zrtsyscalls.c file; 
 * (see implementation of syscallback in "syscall_manager.S" file ) 
*/
void syscall_director(void);


#endif /* _LIB_ZRT_H_ */
