/*
 * utils.h
 *
 *  Created on: 25.12.2012
 *      Author: YaroslavLitvinov
 */

#ifndef __HELPERS_UTILS_H__
#define __HELPERS_UTILS_H__

/*
 * analog of realpath, that allows non existant file/dir as a last 
 * component path. 
 * @param resolved_path if not NULL then result will be copied here, 
 * if it's NULL then it uses malloc.
 * @return -1 If some path components are absent on disk excluding last
 * component part.
 */
char* zrealpath( const char* path, char* resolved_path );

/*convert string to unsingned int, to be used in prolog, not used locale */
uint strtouint_nolocale(const char* str, int base, int *err );

#endif //__HELPERS_UTILS_H__
