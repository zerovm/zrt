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

#include "zrt_defines.h" //CONSTRUCT_L

/*name of constructor*/
#define NVRAM_LOADER nvram_loader


struct NvramLoaderPublicInterface{
    /*at least one observer should be added before NvramLoader::read is invoked*/
    void 
    (*add_observer)(struct NvramLoaderPublicInterface* nvram, 
		    struct MNvramObserver* observer);

    /*read nvram file*/
    int  
    (*read)(struct NvramLoaderPublicInterface* nvram, 
	    const char* nvram_file_name);

    /*parse nvram and handle parsed data inside observers, in case if observer is NULL
      all sections must be parsed, but if provided observer is matched then only
      section appropriate to observer should be parsed.
     *@param observer*/
    void 
    (*parse)(struct NvramLoaderPublicInterface* nvram);

    /*handle parsed nvram data by observer functions, if provided observer is matched 
      then only section appropriate to observer should be parsed.
    @param obj1, obj2, obj3 not mandatory additional parameters for observers*/
    void 
    (*handle)(struct NvramLoaderPublicInterface* nvram, 
	      struct MNvramObserver* observer,
	      void* obj1, void* obj2, void* obj3);
    
    /*@return Section data by name, NULL if not located*/
    struct ParsedRecords* (*section_by_name)(struct NvramLoaderPublicInterface* nvram, 
					     const char* name);
};


struct NvramLoaderPublicInterface* nvram_loader();
void free_nvram_loader();

#endif /* NVRAM_LOADER_H_ */
