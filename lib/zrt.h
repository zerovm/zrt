/*
 * ZeroVM runtime
 *
 *  Created on: Jul 3, 2012
 *      Author: d'b, YaroslavLitvinov
 */

#ifndef ZRT_H_
#define ZRT_H_
#ifndef USER_SIDE
#define USER_SIDE
#endif

#include <stdint.h>
#include "zvm.h"

/* enabling */
//#define DEBUG 0

/*
 * user program entry point. old style function prototyping
 * allows to avoid error when main has an empty arguments list
 * note: arguments still can be passed (see "command_line.c")
 *
 * when (and if) "blob engine" will be developed this cheat will
 * be removed
 */
#define main slave_main
int slave_main();

/* entry point for zrt library sample (see "syscall_manager.S" file) */
void syscall_director(void);

#endif /* ZRT_H_ */
