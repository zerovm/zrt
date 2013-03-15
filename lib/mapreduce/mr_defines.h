/*
 * mr_defines.h
 *
 *  Created on: 14.07.2012
 *      Author: yaroslav
 */

#ifndef MR_DEFINES_H_
#define MR_DEFINES_H_

#ifdef DEBUG
#  define WRITE_FMT_LOG(fmt, args...) {fprintf(stderr, fmt, args); fflush(0);}
#  define WRITE_LOG(str) {fprintf(stderr, "%s\n", str); fflush(0);}
#else
#  define WRITE_FMT_LOG(fmt, args...)
#  define WRITE_LOG(str)
#endif

#endif /* MR_DEFINES_H_ */
