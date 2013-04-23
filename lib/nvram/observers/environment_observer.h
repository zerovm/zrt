/*
 * environment_observer.h
 *
 *  Created on: 22.04.2013
 *      Author: yaroslav
 */

#ifndef __ENVIRONMENT_OBSERVER_H__
#define __ENVIRONMENT_OBSERVER_H__

#define ENVIRONMENT_SECTION_NAME    "env"
#define ENVIRONMENT_PARAM_NAME_KEY  "name"
#define ENVIRONMENT_PARAM_VALUE_KEY "value"

#include "nvram_observer.h"

/*get static interface object not intended to destroy after using*/
struct MNvramObserver* get_env_observer();

#endif /* __ENVIRONMENT_OBSERVER_H__ */
