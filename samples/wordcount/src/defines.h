/*
 * defines.h
 *
 *  Created on: 14.07.2012
 *      Author: yaroslav
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#define DEBUG

#ifdef DEBUG
/*use OUTPUT_HASH_KEYS option only for debugging purposes, output file contains 
 *values and hashes and their size even bigger than input data size */
/* #  define OUTPUT_HASH_KEYS */
#endif //DEBUG

#ifdef DEBUG
#  ifndef WRITE_FMT_LOG
#    define WRITE_FMT_LOG(fmt, args...) fprintf(stderr, fmt, args)
#    define WRITE_LOG(str) fprintf(stderr, "%s\n", str)
#  endif
#else
#  ifndef WRITE_FMT_LOG
#    define WRITE_FMT_LOG(fmt, args...)
#    define WRITE_LOG(str)
#  endif
#endif


#endif /* DEFINES_H_ */
