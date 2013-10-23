/*
 * utils.h
 *
 *  Created on: 25.12.2012
 *      Author: YaroslavLitvinov
 */

#ifndef __HELPERS_UTILS_H__
#define __HELPERS_UTILS_H__

/*
 * create absolute path, for relative path just insert into beginning
 * '/' char, user application can provide relative path, currently any
 * of zrt filesystems does not support relative path, so making
 * absolute path is required. It is not using stat() function opposite
 * to realpath from libc.  In some cases it's better.  
 * @param resolved_path if not NULL then result will be copied here, 
 * if it's NULL then it uses malloc.
 */
char* zrealpath( const char* path, char* resolved_path );

/*convert string to unsingned int, to be used in prolog, not used locale */
uint strtouint_nolocale(const char* str, int base, int *err );

#endif //__HELPERS_UTILS_H__
