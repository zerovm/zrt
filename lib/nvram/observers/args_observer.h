/*
 * args_observer.h
 *
 *  Created on: 3.06.2013
 *      Author: yaroslav
 */

#ifndef __ARGS_OBSERVER_H__
#define __ARGS_OBSERVER_H__

#define ARGS_SECTION_NAME     "args"
#define ARGS_PARAM_VALUE_KEY  "args"

#include "nvram_observer.h"

/*get static interface object not intended to destroy after using*/
struct MNvramObserver* get_arg_observer();

/*fill two-dimensional array by environ vars to be used by prolog */
void get_arg_array(char **args, char* buf, int bufsize);


#endif /* __ARGS_OBSERVER_H__ */
