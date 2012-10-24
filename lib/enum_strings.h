/*
 * enum_strings.h
 * string constants, for debugging purposes
 *
 *  Created on: 22.10.2012
 *      Author: YaroslavLitvinov
 */


#ifndef __ENUM_STRINGS_H__
#define __ENUM_STRINGS_H__

#define _GNU_SOURCE
#include <fcntl.h> //file flags
#include <sys/mman.h>


/*log to zrt debug flags for opening file in human representation*/
void log_file_open_flags( int flags );

/*log to zrt debug mmap prot flags*/
void log_mmap_prot( int prot );

#endif //__ENUM_STRINGS_H__
