/*
 * precache_observer.h
 *
 *  Created on: 6.06.2013
 *      Author: yaroslav
 */

#ifndef PRECACHE_OBSERVER_H_
#define PRECACHE_OBSERVER_H_

#define HANDLE_ONLY_PRECACHE_SECTION get_precache_observer()

#define PRECACHE_SECTION_NAME         "precache"
#define PRECACHE_PARAM_PRECACHE_KEY   "precache"

#include "nvram_observer.h"

/*get static interface, object not intended to destroy after using*/
struct MNvramObserver* get_precache_observer();

#endif /* PRECACHE_OBSERVER_H_ */
