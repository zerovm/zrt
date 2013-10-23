/*
 * parse_path.h
 *
 *  Created on: 27.09.2012
 *      Author: yaroslav
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
