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

#ifndef UNPACK_INTERFACE_H_
#define UNPACK_INTERFACE_H_

struct MountsReader;
struct UnpackInterface;

typedef enum { EUnpackStateOk=0, EUnpackToBigPath=1, EUnpackStateNotImplemented=38 } UnpackState;
typedef enum{ ETypeFile=0, ETypeDir=1 } TypeFlag;

/*should be used by user class to observe readed files*/
struct UnpackObserver{
    /*new entry extracted from archive*/
    int (*extract_entry)( struct UnpackInterface*, TypeFlag type, const char* name, int entry_size );
    //data
    struct MountsInterface* mounts;
};


struct UnpackInterface{
    int (*unpack)( struct UnpackInterface* unpack_if, const char* mount_path );
    //data
    struct MountsReader* mounts_reader;
    struct UnpackObserver* observer;
};

#endif /* UNPACK_INTERFACE_H_ */
