/*
 * parse_path.h
 *
 *  Created on: 27.09.2012
 *      Author: yaroslav
 */

#ifndef PARSE_PATH_H_
#define PARSE_PATH_H_

struct ParsePathObserver{
    int (*callback_parse)(struct ParsePathObserver*, const char *path, int len);
    //data
    void* anyobj;
};


/*return parsed count*/
int parse_path( struct ParsePathObserver* observer, const char *path );

#endif /* PARSE_PATH_H_ */
