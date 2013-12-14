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
