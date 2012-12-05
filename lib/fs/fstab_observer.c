/*
 * fstab_observer.c
 *
 *  Created on: 03.12.2012
 *      Author: yaroslav
 */

#include <stdio.h>

#include "zrtlog.h"
#include "unpack_tar.h" //tar unpacker
#include "stream_reader.h"
#include "fstab_observer.h"
#include "fstab_loader.h"
#include "image_engine.h"

static struct MFstabObserver s_fstat_observer;

int handle_fstab_record(struct FstabLoader* fstab_loader,
            const char* channel_alias, const char* mount_path){
    ZRT_LOG(L_SHORT, "parsed: channel=%s, mount_path=%s", channel_alias, mount_path);
    /*
     * load filesystem from channel with channel_alias name. Content of filesystem is 
     * reading from tar image channel that points to supported archive, tar currently, 
     * read every file and add it contents into MemMount filesystem
     */
    /*create stream reader linked to tar archive that contains filesystem image*/
    struct StreamReader* stream_reader =
            alloc_stream_reader( fstab_loader->channels_mount, channel_alias );

    if ( stream_reader ){
        /*create image loader, passed 1st param: image alias, 2nd param: Root filesystem;
         * Root filesystem passed instead MemMount to reject adding of files into /dev folder;
         * For example if archive contains non empty /dev folder that contents will be ignored*/
        struct ImageInterface* image_loader =
                alloc_image_loader( fstab_loader->transparent_mount );
        /*create archive unpacker, channels_mount is useing to read channel stream*/
        struct UnpackInterface* tar_unpacker =
                alloc_unpacker_tar( stream_reader, image_loader->observer_implementation );

        /*read archive from linked channel and add all contents into Filesystem*/
        int count_files = image_loader->deploy_image( mount_path, tar_unpacker );
        ZRT_LOG( L_SHORT, "From %s archive readed and injected %d files into %s folder of ZRT filesystem",
                channel_alias, count_files, mount_path );

        free_unpacker_tar( tar_unpacker );
        free_image_loader( image_loader );
        free_stream_reader( stream_reader );
    }
    ZRT_LOG_DELIMETER;
    return 0;
}



struct MFstabObserver* get_fstab_observer(){
    s_fstat_observer.handle_fstab_record = handle_fstab_record;
    return &s_fstat_observer;
}


