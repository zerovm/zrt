/*
 * path_utils.h
 *
 *  Created on: 25.12.2012
 *      Author: YaroslavLitvinov
 */

#ifndef __PATH_UTILS_H__
#define __PATH_UTILS_H__

/*
 * alloc absolute path, for relative path just insert into beginning '/' char, 
 * for absolute path just alloc and return. user application can provide relative path, 
 * currently any of zrt filesystems does not supported relative path, so making absolute 
 * path is required.
 */
char* alloc_absolute_path_from_relative( const char* path );


#endif //__PATH_UTILS_H__
