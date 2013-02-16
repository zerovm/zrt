/*
 * nvram_loader.h
 *
 *  Created on: 01.12.2012
 *      Author: YaroslavLitvinov
 */

#ifndef NVRAM_LOADER_H_
#define NVRAM_LOADER_H_

#include <limits.h>

#include "nvram_observer.h"

#define NVRAM_MAX_FILE_SIZE 10240
#define NVRAM_MAX_OBSERVERS_COUNT 5

/*check nvram size validity*/
#if NVRAM_MAX_FILE_SIZE > SSIZE_MAX
#undef NVRAM_MAX_FILE_SIZE
#define NVRAM_MAX_FILE_SIZE SSIZE_MAX
#endif

struct NvramLoader{
    /*at least one observer should be added before NvramLoader::read is invoked*/
    void (*add_observer)(struct NvramLoader* nvram, struct MNvramObserver* observer);
    /*read nvram file and next run NvramLoader::parse*/
    int  (*read)(struct NvramLoader* nvram, const char* nvram_file_name);
    /*parse nvram and handle parsed data inside observers
     *@return parsed records count*/
    int  (*parse)(struct NvramLoader* nvram);
    void (*free)(struct NvramLoader* nvram);

    //data
    char nvram_data[NVRAM_MAX_FILE_SIZE];
    int  nvram_data_size;
    /*observers array, unused cells would be NULL*/
    struct MNvramObserver*  nvram_observers[NVRAM_MAX_OBSERVERS_COUNT];
    struct MountsInterface* channels_mount;       /*filesystem interface*/
    struct MountsInterface* transparent_mount;    /*filesystem interface*/
};

struct NvramLoader* alloc_nvram_loader(
        struct MountsInterface* channels_mount,
        struct MountsInterface* transparent_mount );

void free_nvram_loader(struct NvramLoader* nvram);

#endif /* NVRAM_LOADER_H_ */
