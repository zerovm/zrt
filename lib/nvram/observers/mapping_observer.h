/*
 *
 * Copyright (c) 2013, LiteStack, Inc.
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

#ifndef MAPPING_OBSERVER_H_
#define MAPPING_OBSERVER_H_

#define HANDLE_ONLY_MAPPING_SECTION get_mapping_observer()

#define MAPPING_SECTION_NAME         "mapping"
#define MAPPING_PARAM_CHANNEL_KEY    "channel"
#define MAPPING_PARAM_TYPE_KEY       "mode"

#define MAPPING_PARAM_VALUE_PIPE     "pipe"
#define MAPPING_PARAM_VALUE_CHR      "char"
#define MAPPING_PARAM_VALUE_FILE     "file"

#include "nvram_observer.h"

struct ChannelsModeUpdaterPublicInterface;

void set_mapping_channels_settings_updater( struct ChannelsModeUpdaterPublicInterface 
					    *nvram_mode_setting_updater );

/*get static interface, object not intended to destroy after using*/
struct MNvramObserver* get_mapping_observer();

#endif /* MAPPING_OBSERVER_H_ */
