/*
 * debug_observer.h
 *
 *  Created on: 6.06.2013
 *      Author: yaroslav
 */

#ifndef DEBUG_OBSERVER_H_
#define DEBUG_OBSERVER_H_

#define HANDLE_ONLY_DEBUG_SECTION get_debug_observer()

#define DEBUG_SECTION_NAME         "debug"
#define DEBUG_PARAM_VERBOSITY_KEY  "verbosity"

#include "nvram_observer.h"

/*get static interface, object not intended to destroy after using*/
struct MNvramObserver* get_debug_observer();

#endif /* DEBUG_OBSERVER_H_ */
