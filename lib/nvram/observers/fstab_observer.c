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
#include "unpack_tar.h" //tar unpacker
#include "stream_reader.h"
#include "fstab_observer.h"
#include "nvram.h"
#include "image_engine.h"
#include "conf_parser.h"
#include "conf_keys.h"


#define FSTAB_PARAM_CHANNEL_KEY_INDEX    0
#define FSTAB_PARAM_MOUNTPOINT_KEY_INDEX 1
#define FSTAB_PARAM_ACCESS_KEY_INDEX     2

/*temporarily added to allow writing to tar at exit*/
static struct ParsedRecords s_export_tarrecords; 
static struct MNvramObserver s_fstab_observer;
static struct MNvramObserver *s_inited_observer = NULL;

int save_as_tar(const char *dir_path, const char *tar_path );

void handle_fstab_record(struct MNvramObserver* observer,
			 struct ParsedRecord* record,
			 void* obj1, void* obj2){
    assert(record);

    /*as temp solution, need to do good solution to export tar*/
    memset(&s_export_tarrecords, '\0', sizeof(s_export_tarrecords));

    /*obj1 - only channels filesystem interface*/
    struct MountsInterface* channels_mount = (struct MountsInterface*)obj1;
    /*obj2 - whole filesystem interface*/
    struct MountsInterface* transparent_mount = (struct MountsInterface*)obj2;

    /*get param*/
    char* channel_alias = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[FSTAB_PARAM_CHANNEL_KEY_INDEX], 
		      &channel_alias);
    /*get param*/
    char* mount_path = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[FSTAB_PARAM_MOUNTPOINT_KEY_INDEX], 
		      &mount_path);
    /*get param check*/
    char* access = NULL;
    ALLOCA_PARAM_VALUE(record->parsed_params_array[FSTAB_PARAM_ACCESS_KEY_INDEX], 
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
            alloc_stream_reader( channels_mount, channel_alias );

	if ( stream_reader ){
	    /*create image loader, passed 1st param: image alias, 2nd param: Root filesystem;
	     * Root filesystem passed instead MemMount to reject adding of files into /dev folder;
	     * For example if archive contains non empty /dev folder that contents will be ignored*/
	    struct ImageInterface* image_loader =
                alloc_image_loader( transparent_mount );
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
    /*save files located at mount_path into tar archive*/
    else if ( !strcasecmp(access, FSTAB_VAL_ACCESS_WRITE) ){
	struct ParsedRecord* record = &s_export_tarrecords.records[s_export_tarrecords.count];
	struct ParsedParam* p1 = &record->parsed_params_array[FSTAB_PARAM_CHANNEL_KEY_INDEX];
	struct ParsedParam* p2 = &record->parsed_params_array[FSTAB_PARAM_MOUNTPOINT_KEY_INDEX];
	p1->key_index = FSTAB_PARAM_CHANNEL_KEY_INDEX;
	p1->val = strdup(channel_alias);
	p1->vallen = strlen(channel_alias);
	p2->key_index = FSTAB_PARAM_MOUNTPOINT_KEY_INDEX;
	p2->val = strdup(mount_path);
	p2->vallen = strlen(mount_path);
	s_export_tarrecords.count++;
    }
#endif //FSTAB_SAVE_TAR_ENABLE

    ZRT_LOG_DELIMETER;
}

void handle_tar_export(){
    struct ParsedRecord* current;
    int i;
    for ( i=0; i < s_export_tarrecords.count; i++ ){
	current = &s_export_tarrecords.records[i];
	/*get param*/
	char* channel_alias = NULL;
	ALLOCA_PARAM_VALUE(current->parsed_params_array[FSTAB_PARAM_CHANNEL_KEY_INDEX], 
			   &channel_alias);
	/*get param*/
	char* mount_path = NULL;
	ALLOCA_PARAM_VALUE(current->parsed_params_array[FSTAB_PARAM_MOUNTPOINT_KEY_INDEX], 
			   &mount_path);
	int res = save_as_tar(mount_path, channel_alias);
	ZRT_LOG(L_SHORT, "save_as_tar res=%d, dirpath=%s, tar_path=%s", 
		res, mount_path, channel_alias);
    }
}

static void cleanup_fstab_observer( struct MNvramObserver* obs){
}

struct MNvramObserver* get_fstab_observer(){
    if ( s_inited_observer ) 
	return s_inited_observer;
    struct MNvramObserver* self = &s_fstab_observer;
    ZRT_LOG(L_INFO, "Create observer for section: %s", FSTAB_SECTION_NAME);
    /*setup section name*/
    strncpy(self->observed_section_name, FSTAB_SECTION_NAME, NVRAM_MAX_SECTION_NAME_LEN);
    /*setup section keys*/
    keys_construct(&self->keys);
    /*add keys and check returned key indexes that are the same as expected*/
    int key_index;
    /*check parameters*/
    key_index = self->keys.add_key(&self->keys, FSTAB_PARAM_CHANNEL_KEY);
    assert(FSTAB_PARAM_CHANNEL_KEY_INDEX==key_index);
    key_index = self->keys.add_key(&self->keys, FSTAB_PARAM_MOUNTPOINT_KEY);
    assert(FSTAB_PARAM_MOUNTPOINT_KEY_INDEX==key_index);
    key_index = self->keys.add_key(&self->keys, FSTAB_PARAM_ACCESS_KEY);
    assert(FSTAB_PARAM_ACCESS_KEY_INDEX==key_index);

    /*setup functions*/
    s_fstab_observer.handle_nvram_record = handle_fstab_record;
    s_fstab_observer.cleanup = cleanup_fstab_observer;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", FSTAB_SECTION_NAME);
    s_inited_observer = &s_fstab_observer;
    return s_inited_observer;
}

#endif //FSTAB_CONF_ENABLE
