/*
 *
 * Copyright (c) 2013, LiteStack, Inc.
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

/*@return 0 OK, -1 error*/
int test_strtouint_nolocale();

#endif //__HELPERS_UTILS_H__
