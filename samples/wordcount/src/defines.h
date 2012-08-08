/*
 * defines.h
 *
 *  Created on: 14.07.2012
 *      Author: yaroslav
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#ifdef DEBUG
#  ifndef WRITE_FMT_LOG
#    define WRITE_FMT_LOG(fmt, args...) fprintf(stderr, fmt, args)
#    define WRITE_LOG(str) fprintf(stdout, "%s\n", str)
#  endif
#else
#  ifndef WRITE_FMT_LOG
#    define WRITE_FMT_LOG(fmt, args...)
#    define WRITE_LOG(str)
#  endif
#endif

#endif /* DEFINES_H_ */
