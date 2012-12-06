/*
 * fstab_loader.h
 * fstab file is intended to inject ZRT filesystem with content that related to
 * list of tar images specified in fstab config file.
 * Tarball images will processed in the same order as specified in fstab.
 *
 *  Created on: 01.12.2012
 *      Author: YaroslavLitvinov
 */

#ifndef FSTAB_LOADER_H_
#define FSTAB_LOADER_H_

#include <limits.h>

#include "fstab_observer.h"

#define FSTAB_MAX_FILE_SIZE 10240

/*check fstab size validity*/
#if FSTAB_MAX_FILE_SIZE > SSIZE_MAX
#undef FSTAB_MAX_FILE_SIZE
#define FSTAB_MAX_FILE_SIZE SSIZE_MAX
#endif

#define RECORD_PARAMS_COUNT 2
#define PARAM_CHANNEL_KEY "channel"
#define PARAM_MOUNTPOINT_KEY "mountpoint"


struct FstabLoader{
    /*read fstab file*/
    int (*read)(struct FstabLoader* fstab, const char* fstab_file_name);
    /*parse fstab and handle parsed data inside observer*/
    int (*parse)(struct FstabLoader* fstab, struct MFstabObserver* observer);

    //data
    char fstab_contents[FSTAB_MAX_FILE_SIZE];
    int fstab_size;
    struct MFstabObserver*  fstab_observer;
    struct MountsInterface* channels_mount;
    struct MountsInterface* transparent_mount;
};

struct FstabLoader* alloc_fstab_loader(
        struct MountsInterface* channels_mount,
        struct MountsInterface* transparent_mount );

void free_fstab_loader(struct FstabLoader* fstab);

#endif /* FSTAB_LOADER_H_ */
