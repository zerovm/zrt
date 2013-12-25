/*
 * mount_specific_implem.c
 * Interface for functions whose implemetation is different for
 * various mounts: channels and MemMount;
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
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


#include "mount_specific_implem.h"

#include <stdlib.h>

struct MountSpecificImplemPublicInterface*
mount_specific_construct( struct MountSpecificImplemPublicInterface* specific_implem_interface,
			  struct ChannelsArrayPublicInterface* channels_array ){
    struct MountSpecificImplem* this = malloc(sizeof(struct MountSpecificImplem));
    /*set functions*/
    this->public_ = *specific_implem_interface;
    /*set data members*/
    this->channels_array = channels_array;
    return (struct MountSpecificImplemPublicInterface*)this;
}

