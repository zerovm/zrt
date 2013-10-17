/*
 * fstab_observer.h
 *
 *  Created on: 03.12.2012
 *      Author: yaroslav
 */

#ifndef FSTAB_OBSERVER_H_
#define FSTAB_OBSERVER_H_

#include "nvram_observer.h"

enum { EFstabStageMountFirst, EFstabStageRemount };

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

/* If mount_stage is equal to FSTAB_MOUNT_FIRST_STAGE then always return 1,
 * if fstab_stage value is equal to FSTAB_REMOUNT_STAGE then return 1 only 
 * in case if removable_record is 1 */
#define IS_NEED_TO_HANDLE_FSTAB_RECORD(mount_stage, removable_record)	\
    ((EFstabStageMountFirst)==(mount_stage))? 1:			\
    ((EFstabStageRemount)==(mount_stage)) && 0!=(removable_record)? 1 : 0

/*get static interface object not intended to destroy after using*/
struct MNvramObserver* get_fstab_observer();
void handle_tar_export();

#endif /* FSTAB_OBSERVER_H_ */
