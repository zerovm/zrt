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

#ifndef DEBUG_OBSERVER_H_
#define DEBUG_OBSERVER_H_

#define HANDLE_ONLY_DEBUG_SECTION get_debug_observer()

#define DEBUG_SECTION_NAME         "debug"
#define DEBUG_PARAM_VERBOSITY_KEY  "verbosity"

#include "nvram_observer.h"

/*get static interface, object not intended to destroy after using*/
struct MNvramObserver* get_debug_observer();

#endif /* DEBUG_OBSERVER_H_ */
