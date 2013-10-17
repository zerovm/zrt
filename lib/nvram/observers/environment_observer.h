/*
 * environment_observer.h
 *
 *  Created on: 22.04.2013
 *      Author: yaroslav
 */

#ifndef __ENVIRONMENT_OBSERVER_H__
#define __ENVIRONMENT_OBSERVER_H__

#define HANDLE_ONLY_ENV_SECTION get_env_observer()

#define ENVIRONMENT_SECTION_NAME    "env"
#define ENVIRONMENT_PARAM_NAME_KEY  "name"
#define ENVIRONMENT_PARAM_VALUE_KEY "value"

#include "nvram_observer.h"

/*get static interface object not intended to destroy after using*/
struct MNvramObserver* get_env_observer();

/*fill two-dimensional array by environ vars to be used by prolog */
void get_env_array(char **envs, char* buf, int bufsize);


#endif /* __ENVIRONMENT_OBSERVER_H__ */
