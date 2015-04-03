/*
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

#ifndef PARSE_PATH_H_
#define PARSE_PATH_H_

struct ParsePathObserver{
    void (*callback_parse)(struct ParsePathObserver*, const char *path, int len);
    //data
    void* anyobj;
};

/* check if directory related to path already cached. 
 * @return 1 if dir created and dir name added to cache, 
 * or return 0 if dir cached and it means that it already created*/
int create_dir_and_cache_name( const char* path, int len );

/*return parsed count*/
int parse_path( struct ParsePathObserver* observer, const char *path );

#endif /* PARSE_PATH_H_ */
