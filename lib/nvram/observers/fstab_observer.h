/*
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

#ifndef FSTAB_OBSERVER_H_
#define FSTAB_OBSERVER_H_

#include "nvram_observer.h"
#include "conf_parser.h" //struct ParsedRecord

#define HANDLE_ONLY_FSTAB_SECTION get_fstab_observer()

#define FSTAB_SECTION_NAME         "fstab"
#define FSTAB_PARAM_CHANNEL_KEY    "channel"
#define FSTAB_PARAM_MOUNTPOINT_KEY "mountpoint"
#define FSTAB_PARAM_ACCESS_KEY     "access"
#define FSTAB_PARAM_REMOVABLE         "removable"

#define FSTAB_VAL_ACCESS_READ      "ro"  /*for injecting files into FS*/
#define FSTAB_VAL_ACCESS_WRITE     "wo"  /*for copying files into image*/

#define FSTAB_VAL_REMOVABLE_YES       "yes"
#define FSTAB_VAL_REMOVABLE_NO        "no"

enum {EFstabMountWaiting, EFstabMountProcessing, EFstabMountComplete};
struct FstabRecordContainer{
    struct ParsedRecord mount;
    int mount_status; /* EFstabMountWaiting, EFstabMountProcessing, EFstabMountComplete */
};

/*new fstab observer is derived from nvram observer*/
struct FstabObserver {
    struct MNvramObserver base;
    /*derived function "handle_nvram_record(...)"*/
    /*export fs contents into tar archive, in according to fstab record with access=rw*/
    void (*mount_export)(struct FstabObserver* observer);
    /*import tar archive  into maountpoint path, related to fstab record with access=ro*/
    void (*mount_import)(struct FstabObserver* observer, 
			 struct FstabRecordContainer* record);
    /*Erase previously loaded from nvram config fstab records, nvram
      can be loaded multiple times during zfork()*/
    void (*erase_old_mounts)(struct FstabObserver* observer);
    /*Say to removable mounts that they need to be remounted*/
    void (*reset_removable)(struct FstabObserver* observer);
    /* Locate ParsedRecord with mountpoint and mount status matched
     * @param alias 
     * @param mount_status 
     * @return index of matched record, -1 if not located*/
    struct FstabRecordContainer* (*locate_postpone_mount)(struct FstabObserver* observer, 
						  const char* alias, int mount_status);
    /*new fields for postponed lazy mount, tar export at exit;
     *expected that array will get new items during fstab handling*/
    struct FstabRecordContainer* postpone_mounts_array;
    int postpone_mounts_count;
};

/*get static interface object not intended to destroy after using*/
struct FstabObserver* get_fstab_observer();

#endif /* FSTAB_OBSERVER_H_ */
