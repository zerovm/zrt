/*
 * ZeroVM runtime
 *
 *  Created on: Jul 3, 2012
 *      Author: d'b, YaroslavLitvinov
 */

#ifndef _LIB_ZRT_H_
#define _LIB_ZRT_H_
#ifndef USER_SIDE
#define USER_SIDE
#endif

#include <stdint.h>


#ifdef DEBUG
#  define WRITE_FMT_LOG(fmt, args...) fprintf(stderr, fmt, args)
#  define WRITE_LOG(str) fprintf(stderr, "%s\n", str)
#else
#  define WRITE_FMT_LOG(fmt, args...)
#  define WRITE_LOG(str)
#endif


/*
 * @return channels count*/
const struct ZVMChannel *zvm_channels_c( int *channels_count );

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

#endif /* _LIB_ZRT_H_ */
