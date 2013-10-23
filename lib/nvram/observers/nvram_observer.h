/*
 * It's a generic observer interface to be used with NvramLoader
 * nvram_observer.h
 *
 *  Created on: 03.12.2012
 *      Author: yaroslav
 */

#ifndef NVRAM_OBSERVER_H_
#define NVRAM_OBSERVER_H_

#include "nvram.h"
#include "conf_keys.h"

/*forward declarations*/
struct NvramLoader;
struct ParsedRecord;

/*Handle single parsed record
 *Base nvram observer interface should be derived by another observers*/
struct MNvramObserver{
    /*Handle parsed nvram record data
     *@param record 
     *@param obj1, obj2, obj3 can be any object expected by section handler, can be NULL*/
    void (*handle_nvram_record)(struct MNvramObserver* observer,
				struct ParsedRecord* record,
				void* obj1, void* obj2, void* obj3);
    /*private 
     *Check all record params and return 0 if it's valid, return -1 if otherwise*/
    int (*is_valid_record)(struct MNvramObserver* observer, struct ParsedRecord* record);
    /*it's a section name in nvram config file which corresponds to observer;
     *only if section name in config file will equal to folowing section name
     *then observer will handle data belonging to this section*/
    char observed_section_name[NVRAM_MAX_SECTION_NAME_LEN];

    struct KeyList keys;
};

#endif /* NVRAM_OBSERVER_H_ */
