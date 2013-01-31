/*
 * zrt.h
 * ZeroVM runtime, it's not required to include this file into user code
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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
