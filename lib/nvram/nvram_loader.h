/*
 * Nvram config file loader 
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


#ifndef NVRAM_LOADER_H_
#define NVRAM_LOADER_H_

#include <limits.h>

#include "nvram.h"
#include "nvram_observer.h"
#include "conf_parser.h"

struct NvramLoader{
    /*at least one observer should be added before NvramLoader::read is invoked*/
    void 
    (*add_observer)(struct NvramLoader* nvram, struct MNvramObserver* observer);

    /*read nvram file*/
    int  
    (*read)(struct NvramLoader* nvram, const char* nvram_file_name);

    /*parse nvram and handle parsed data inside observers, in case if observer is NULL
      all sections must be parsed, but if provided observer is matched then only
      section appropriate to observer should be parsed.
     *@param observer*/
    void 
    (*parse)(struct NvramLoader* nvram);

    /*handle parsed nvram data by observer functions, if provided observer is matched 
      then only section appropriate to observer should be parsed.
    @param obj1, obj2, obj3 not mandatory additional parameters for observers*/
    void 
    (*handle)(struct NvramLoader* nvram, struct MNvramObserver* observer,
	      void* obj1, void* obj2, void* obj3);
    
    /*@return Section data by name, NULL if not located*/
    struct ParsedRecords* 
    (*section_by_name)(struct NvramLoader* nvram, const char* name);

    //data
    char nvram_data[NVRAM_MAX_FILE_SIZE];
    int  nvram_data_size;
    struct ParsedRecords parsed_sections[NVRAM_MAX_SECTIONS_COUNT];
    int parsed_sections_count;
    /*array of pointers to observer objects, unused cells would be NULL*/
    struct MNvramObserver* nvram_observers[NVRAM_MAX_OBSERVERS_COUNT];
};

struct NvramLoader* construct_nvram_loader(struct NvramLoader* nvram_loader);
void free_nvram_loader(struct NvramLoader* nvram);

/* read nvram file, set all observers, parse;
 * if adding new sections then changes needed here;
 * @return 0 if read ok*/
int nvram_read_parse( struct NvramLoader* nvram );

#endif /* NVRAM_LOADER_H_ */
