/*
 * fstab_observer.c
 *
 *  Created on: 03.12.2012
 *      Author: yaroslav
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "zrt_config.h"

/*switch off all of code if it's deisabled*/
#ifdef FSTAB_CONF_ENABLE

#include "zrtlog.h"
//#include "tar.h"
#include "unpack_tar.h" //tar unpacker
#include "stream_reader.h"
#include "fstab_observer.h"
#include "nvram_loader.h"
#include "image_engine.h"
#include "conf_parser.h"
#include "conf_keys.h"


#define FSTAB_PARAM_CHANNEL_KEY_INDEX    0
#define FSTAB_PARAM_MOUNTPOINT_KEY_INDEX 1
#define FSTAB_PARAM_ACCESS_KEY_INDEX     2

static struct MNvramObserver s_fstat_observer;

int save_as_tar(const char *dir_path, const char *tar_path );

void handle_fstab_record(struct MNvramObserver* observer,
			 struct NvramLoader* nvram_loader,
			 struct ParsedRecord* record){
    /*get param*/
    char* channel_alias = NULL;
    ALLOC_PARAM_VALUE(record->parsed_params_array[FSTAB_PARAM_CHANNEL_KEY_INDEX], 
		      &channel_alias);
    /*get param*/
    char* mount_path = NULL;
    ALLOC_PARAM_VALUE(record->parsed_params_array[FSTAB_PARAM_MOUNTPOINT_KEY_INDEX], 
		      &mount_path);
    /*get param check*/
    char* access = NULL;
    ALLOC_PARAM_VALUE(record->parsed_params_array[FSTAB_PARAM_ACCESS_KEY_INDEX], 
		      &access);
    ZRT_LOG(L_SHORT, "fstab record: channel=%s, mount_path=%s, access=%s", 
	    channel_alias, mount_path, access);

    /*for injecting files into FS*/
    if ( !strcasecmp(access, FSTAB_VAL_ACCESS_READ) ){
	/*
	 * load filesystem from channel having which name is channel_alias. 
	 * Content of filesystem is reading from tar image channel that points to 
	 * supported archive, tar currently, read every file and add it contents 
	 * into MemMount filesystem
	 */
	/*create stream reader linked to tar archive that contains filesystem image*/
	struct StreamReader* stream_reader =
            alloc_stream_reader( nvram_loader->channels_mount, channel_alias );

	if ( stream_reader ){
	    /*create image loader, passed 1st param: image alias, 2nd param: Root filesystem;
	     * Root filesystem passed instead MemMount to reject adding of files into /dev folder;
	     * For example if archive contains non empty /dev folder that contents will be ignored*/
	    struct ImageInterface* image_loader =
                alloc_image_loader( nvram_loader->transparent_mount );
	    /*create archive unpacker, channels_mount is useing to read channel stream*/
	    struct UnpackInterface* tar_unpacker =
                alloc_unpacker_tar( stream_reader, image_loader->observer_implementation );

	    /*read archive from linked channel and add all contents into Filesystem*/
	    int count_files = image_loader->deploy_image( mount_path, tar_unpacker );
	    ZRT_LOG( L_SHORT, 
		     "From %s archive readed and injected %d files "
		     "into %s folder of ZRT filesystem",
		     channel_alias, count_files, mount_path );

	    free_unpacker_tar( tar_unpacker );
	    free_image_loader( image_loader );
	    free_stream_reader( stream_reader );
	}
    }
#ifdef FSTAB_SAVE_TAR_ENABLE
    /*for injecting files into FS*/
    else if ( !strcasecmp(access, FSTAB_VAL_ACCESS_WRITE) ){
	int res = save_as_tar(mount_path, channel_alias );
	ZRT_LOG(L_SHORT, "save_as_tar res=%d, dirpath=%s, tar_path=%s", 
		res, mount_path, channel_alias);
    }
#endif //FSTAB_SAVE_TAR_ENABLE

    free(channel_alias);
    free(mount_path);
    ZRT_LOG_DELIMETER;
}

void cleanup_fstab_observer( struct MNvramObserver* obs){
    assert(obs);
    obs->keys->free( obs->keys );
}

struct MNvramObserver* get_fstab_observer(){
    struct MNvramObserver* self = &s_fstat_observer;
    ZRT_LOG(L_SHORT, "prepare section: %s", FSTAB_SECTION_NAME);
    /*setup section name*/
    strcpy(self->observed_section_name, FSTAB_SECTION_NAME);
    /*setup section keys*/
    self->keys = alloc_key_list();
    /*add keys and check returned key indexes are the same as will be used in futher*/
    int key_index;
    /*check parameters*/
    key_index = self->keys->add_key(self->keys, FSTAB_PARAM_CHANNEL_KEY);
    assert(FSTAB_PARAM_CHANNEL_KEY_INDEX==key_index);
    key_index = self->keys->add_key(self->keys, FSTAB_PARAM_MOUNTPOINT_KEY);
    assert(FSTAB_PARAM_MOUNTPOINT_KEY_INDEX==key_index);
    key_index = self->keys->add_key(self->keys, FSTAB_PARAM_ACCESS_KEY);
    assert(FSTAB_PARAM_ACCESS_KEY_INDEX==key_index);

    /*setup functions*/
    s_fstat_observer.handle_nvram_record = handle_fstab_record;
    s_fstat_observer.cleanup = cleanup_fstab_observer;
    return &s_fstat_observer;
}

#endif //FSTAB_CONF_ENABLE
