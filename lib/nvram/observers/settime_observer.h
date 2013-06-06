/*
 * settime_observer.h
 *
 *  Created on: 05.06.2013
 *      Author: yaroslav
 */

#ifndef SETTIME_OBSERVER_H_
#define SETTIME_OBSERVER_H_

#define TIME_SECTION_NAME         "time"
#define TIME_PARAM_SECONDS_KEY    "seconds"

#include "nvram_observer.h"

/*get static interface, object not intended to destroy after using*/
struct MNvramObserver* get_settime_observer();

#endif /* SETTIME_OBSERVER_H_ */
