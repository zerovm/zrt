/*
 * settime_observer.h
 *
 *  Created on: 6.06.2013
 *      Author: yaroslav
 */

#ifndef MAPPING_OBSERVER_H_
#define MAPPING_OBSERVER_H_

#define MAPPING_SECTION_NAME         "mapping"
#define MAPPING_PARAM_CHANNEL_KEY    "channel"
#define MAPPING_PARAM_TYPE_KEY       "mode"

#define MAPPING_PARAM_VALUE_PIPE     "pipe"
#define MAPPING_PARAM_VALUE_CHR      "char"
#define MAPPING_PARAM_VALUE_FILE     "file"

#include "nvram_observer.h"

/*get static interface, object not intended to destroy after using*/
struct MNvramObserver* get_mapping_observer();

#endif /* MAPPING_OBSERVER_H_ */
