/*
 * unpack_interface.h
 *
 *  Created on: 26.09.2012
 *      Author: yaroslav
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
