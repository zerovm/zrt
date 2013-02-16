/*
 * It's a generic observer interface to be used with NvramLoader
 * nvram_observer.h
 *
 *  Created on: 03.12.2012
 *      Author: yaroslav
 */

#ifndef NVRAM_OBSERVER_H_
#define NVRAM_OBSERVER_H_

#define MAX_OBSERVER_SECTION_NAME_LEN 50
#define MAX_KEYS_COUNT 4

/*forward declarations*/
struct NvramLoader;
struct ParsedRecord;

/*Handle single parsed record
 *Base nvram observer interface should be derived by another observers*/
struct MNvramObserver{
    /*Handle parsed nvram record data
     *@param keys 
     *@param record */
    void (*handle_nvram_record)(struct MNvramObserver* observer,
				struct NvramLoader* nvram_loader,
				struct ParsedRecord* record );
    void (*cleanup)(struct MNvramObserver* obs);
    /*it's a section name in nvram config file which corresponds to observer;
     *only if section name in config file will equal to folowing section name
     *then observer will handle data belonging to this section*/
    char observed_section_name[MAX_OBSERVER_SECTION_NAME_LEN];

    struct KeyList* keys;
};

#endif /* NVRAM_OBSERVER_H_ */
