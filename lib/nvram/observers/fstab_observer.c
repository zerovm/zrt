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


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "zrt_defines.h"

/*switch off all of code if it's deisabled*/

#include "zrtlog.h"
#include "unpack_tar.h" //tar unpacker
#include "mounts_reader.h"
#include "fstab_observer.h"
#include "fuseglue.h"
#include "nvram.h"
#include "image_engine.h"
#include "conf_parser.h"
#include "conf_keys.h"
#include "args_observer.h" /*prepare fusefs two-dimensional array with arguments*/


#define FSTAB_PARAM_CHANNEL_KEY_INDEX    0
#define FSTAB_PARAM_MOUNTPOINT_KEY_INDEX 1
#define FSTAB_PARAM_ACCESS_KEY_INDEX     2
#define FSTAB_PARAM_REMOVABLE_KEY_INDEX  3
#define FSTAB_PARAM_FSNAME_KEY_INDEX     4

#define GET_FSTAB_PARAMS(record, alias_p, mountpath_p, access_p, removable_p, fsname_p) \
    GET_PARAM_VALUE(record, FSTAB_PARAM_CHANNEL_KEY_INDEX,    alias_p); \
    GET_PARAM_VALUE(record, FSTAB_PARAM_MOUNTPOINT_KEY_INDEX, mountpath_p); \
    GET_PARAM_VALUE(record, FSTAB_PARAM_ACCESS_KEY_INDEX,     access_p); \
    GET_PARAM_VALUE(record, FSTAB_PARAM_REMOVABLE_KEY_INDEX,  removable_p); \
    GET_PARAM_VALUE(record, FSTAB_PARAM_FSNAME_KEY_INDEX,     fsname_p);

/*save directory contents into tar archive
 *Function taken from libports/tar-1.11.8 */
int save_as_tar(const char *dir_path, const char *tar_path );

static struct FstabObserver    s_fstab_observer;
static struct FstabObserver*   s_inited_observer = NULL;
static int s_updated_fstab_records = 0; /*At erase_old_mounts call it assigns value=1, that means fstab re-reading*/
//external objects
static struct MountsPublicInterface* s_channels_mount=NULL;
static struct MountsPublicInterface* s_transparent_mount=NULL;


int handle_is_valid_record(struct MNvramObserver* observer, struct ParsedRecord* record){
    /*get all params*/
    char* channel_alias = NULL;
    char* mount_path = NULL;
    char* access = NULL;
    char* removable = NULL;
    char* fsname = NULL;
    GET_FSTAB_PARAMS(record, &channel_alias, &mount_path, &access, &removable, &fsname);

    if ( ( !strcmp(access, FSTAB_VAL_ACCESS_WRITE) || !strcmp(access, FSTAB_VAL_ACCESS_READ) ) &&
	 ( !strcmp(removable, FSTAB_VAL_REMOVABLE_YES) || !strcmp(removable, FSTAB_VAL_REMOVABLE_NO) ))
	return 0;
    else return 1;	
}

void handle_fstab_record(struct MNvramObserver* observer,
			 struct ParsedRecord* record,
			 void* obj1, void* obj2, void* obj3){
    assert(record);
    struct FstabObserver* fobserver = (struct FstabObserver*)observer;
    s_channels_mount = (struct MountsPublicInterface*)obj1;     /*obj1 - channels filesystem interface*/
    s_transparent_mount = (struct MountsPublicInterface*)obj2;  /*obj2 - whole filesystem interface*/

    if ( observer->is_valid_record(observer, record) != 0 ) return; /*skip record invalid*/

    /*check array*/
    if ( fobserver->postpone_mounts_array == NULL ){
	assert( fobserver->postpone_mounts_count ==0 );
    }

    /*extend array & add record to mounts array
      no checks for duplicated items doing*/
    ++fobserver->postpone_mounts_count;
    fobserver->postpone_mounts_array 
	= realloc(fobserver->postpone_mounts_array, 
		  sizeof(*fobserver->postpone_mounts_array)*fobserver->postpone_mounts_count);
    assert(fobserver->postpone_mounts_array != NULL);
    /*record added into postopne mounts list and must be handled later*/
    struct FstabRecordContainer* record_container = &fobserver->postpone_mounts_array[ fobserver->postpone_mounts_count -1 ];
    record_container->mount_status = EFstabMountWaiting;
    copy_record(record, &record_container->mount);

    /*get all params*/
    char* channel_alias = NULL;
    char* mount_path = NULL;
    char* access = NULL;
    char* removable = NULL;
    char* fsname = NULL;
    GET_FSTAB_PARAMS(record, &channel_alias, &mount_path, &access, &removable, &fsname);

    int write=0;     
    if ( !strcmp(access, FSTAB_VAL_ACCESS_WRITE) )  write =1;

    /*if archivemount is available and want mount tar in read-only mode*/
    if ( !strcasecmp( fsname, "tar") && !strcmp(access, FSTAB_VAL_ACCESS_READ) ){
        int    archivemount_argc;
        char **archivemount_argv;
        fs_main archivemount_entrypoint =
            fusefs_entrypoint_get_args_by_fsname("archivemount", write, 
                                                 channel_alias, mount_path,
                                             &archivemount_argc, &archivemount_argv);
        /*If archivemount is available then use it for tar mounting*/
        if ( archivemount_entrypoint ){
            /*mount fusefs*/
            int mount_res = fuse_main_wrapper(mount_path, archivemount_entrypoint,
                                              archivemount_argc, archivemount_argv);
            if (mount_res!=0){
                ZRT_LOG(L_ERROR, "fuse_main_wrapper err=%d", mount_res);
            }
        }
        /*If no archimemount available and for compatibility use old approach.
          For first fstab handling (s_updated_fstab_records=0) after
          checks try mount channel with keys access=ro, removable=no*/
        else if ( !s_updated_fstab_records  ){
            fobserver->mount_import(fobserver, record_container);
        }
    }
    /*Mount all another file systems*/
    else{
        int    fs_argc=0;
        char **fs_argv;
        fs_main fs_entrypoint =
            fusefs_entrypoint_get_args_by_fsname(fsname, write, 
                                                 channel_alias, mount_path,
                                                 &fs_argc, &fs_argv);

        /*If fuse extensions are available and trying mount tar archive then use archivemount*/
        if (   fs_entrypoint != NULL ){
            /*mount fusefs*/
            int mount_res = fuse_main_wrapper(mount_path, fs_entrypoint, fs_argc, fs_argv);
            if (mount_res!=0){
                ZRT_LOG(L_ERROR, "fuse_main_wrapper err=%d", mount_res);
            }
        }
    }


    ZRT_LOG(L_SHORT, "fstab record channel=%s, mount_path=%s, access=%s, removable=%s, fsname=%s",
	    channel_alias, mount_path, access, removable, fsname);
}

void handle_mount_export(struct FstabObserver* observer){
    struct FstabRecordContainer* record;
    int i;
    for ( i=0; i < observer->postpone_mounts_count; i++ ){
	record = &observer->postpone_mounts_array[i];

	/*get all params*/
	char* channel_alias = NULL;
	char* mount_path = NULL;
	char* access = NULL;
	char* removable = NULL;
	char* fsname = NULL;
	GET_FSTAB_PARAMS(&record->mount, &channel_alias, &mount_path, &access, &removable, &fsname);

	/*save files located at mount_path into tar archive*/
	if ( !strcmp(access, FSTAB_VAL_ACCESS_WRITE) ){
	    int res = save_as_tar(mount_path, channel_alias);
	    ZRT_LOG(L_SHORT, "save_as_tar res=%d, dirpath=%s, tar_path=%s", 
		    res, mount_path, channel_alias);
	}
    }
}

void handle_mount_import(struct FstabObserver* observer, struct FstabRecordContainer* record){
    assert(s_channels_mount != NULL);
    assert(s_transparent_mount != NULL);
    if ( record != NULL ){
        /*get all params*/
        char* channel_alias = NULL;
        char* mount_path = NULL;
        char* access = NULL;
        char* removable = NULL;
        char* fsname = NULL;
        GET_FSTAB_PARAMS(&record->mount, &channel_alias, &mount_path, &access, &removable, &fsname);
        int removable_record = !strcasecmp( removable, FSTAB_VAL_REMOVABLE_YES);

        /* In case if we need to inject files into FS.*/
        if ( !strcmp(access, FSTAB_VAL_ACCESS_READ) && 
             EFstabMountWaiting == record->mount_status &&
             ( removable_record || (!s_updated_fstab_records && !removable_record) ) ){
            /*
             * inject tar contents related to record into mount_path folder of filesystem;
             * Content of filesystem is reading from supported archive type linked to channel, 
             * currently TAR can be mounted, into MemMount filesystem
             */
            ZRT_LOG(L_SHORT, "mount now: channel=%s, mount_path=%s, access=%s, removable=%s, fsname=%s",
                    channel_alias, mount_path, access, removable, fsname);
            record->mount_status = EFstabMountProcessing;
            /*create mounts reader linked to tar archive that contains filesystem image,
              it call "read" from MountsPublicInterface and don't call "read" function from posix layer*/
            struct MountsReader* mounts_reader =
                alloc_mounts_reader( s_channels_mount, channel_alias );

            if ( mounts_reader ){
                /*create image loader, passed 1st param: image alias, 2nd param: Root filesystem;
                 * Root filesystem passed instead MemMount to reject adding of files into /dev folder;
                 * For example if archive contains non empty /dev folder that contents will be ignored*/
                struct ImageInterface* image_loader =
                    alloc_image_loader( s_transparent_mount );
		/*create archive unpacker*/
		struct UnpackInterface* tar_unpacker =
		    alloc_unpacker_tar( mounts_reader, image_loader->observer_implementation );

		/*read archive from linked channel and add all contents into Filesystem*/
		int inject_res = image_loader->deploy_image( mount_path, tar_unpacker );
		record->mount_status = EFstabMountComplete;
		if ( inject_res >=0  ){
		    ZRT_LOG( L_SHORT, 
			     "From %s archive readed and injected %d files "
			     "into %s folder of ZRT filesystem",
			     channel_alias, inject_res, mount_path );
		}
		else{
		    ZRT_LOG( L_ERROR, "Error %d occured while injecting files from %s archive", 
			     inject_res, channel_alias );
		}
		free_unpacker_tar( tar_unpacker );
		free_image_loader( image_loader );
		free_mounts_reader( mounts_reader );
	    }
	}
    }
}

void handle_erase_old_mounts(struct FstabObserver* observer){
    s_updated_fstab_records = 1;
    observer->postpone_mounts_count = 0;
    free(observer->postpone_mounts_array), observer->postpone_mounts_array = NULL;
}

void handle_reset_removable(struct FstabObserver* observer){
    struct FstabRecordContainer* record_container;
    int i;
    for ( i=0; i < observer->postpone_mounts_count; i++ ){
	if ( (record_container = &observer->postpone_mounts_array[i]) != NULL ){
	    /*get all params*/
	    char* channel_alias = NULL;
	    char* mount_path = NULL;
	    char* access = NULL;
	    char* removable = NULL;
	    char* fsname = NULL;
	    GET_FSTAB_PARAMS(&record_container->mount, &channel_alias, &mount_path, &access, &removable, &fsname);
	    int removable_record = !strcasecmp( removable, FSTAB_VAL_REMOVABLE_YES);

	    /* In case if we need to inject files into FS.  
               case 1: do it always at session start (flag s_updated_fstab_records=0); 
               case 2: do it for records with flag removable=yes if nvram
               re-readed (flag s_updated_fstab_records=1)*/
	    if ( !s_updated_fstab_records ||
		 (!strcmp(access, FSTAB_VAL_ACCESS_READ) && removable_record != 0) ){
		record_container->mount_status = EFstabMountWaiting;
	    }
	    else{
		record_container->mount_status = EFstabMountComplete;
	    }
	}
    }
}

struct FstabRecordContainer* 
handle_locate_postpone_mount(struct FstabObserver* this, const char* alias, int mount_status){
    struct FstabRecordContainer* record_container;
    struct ParsedRecord* record;
    char* mountpoint;
    int i;
    for ( i=0; i < this->postpone_mounts_count; i++ ){
	record_container = &this->postpone_mounts_array[i];
	record = &record_container->mount;
	GET_PARAM_VALUE(record, FSTAB_PARAM_MOUNTPOINT_KEY_INDEX, &mountpoint);
	if ( !strncmp(mountpoint, alias, strlen(mountpoint)) &&
	     mount_status == record_container->mount_status ){
	    /*matched*/
	    return record_container;
	}
    }
    return NULL; /*not located*/
}

struct FstabObserver* get_fstab_observer(){
    if ( s_inited_observer != NULL ) 
	return s_inited_observer;
    struct FstabObserver* self = &s_fstab_observer;
    ZRT_LOG(L_INFO, "Create observer for section: %s", FSTAB_SECTION_NAME);
    /*setup section name*/
    strncpy(self->base.observed_section_name, FSTAB_SECTION_NAME, NVRAM_MAX_SECTION_NAME_LEN);
    /*setup section keys*/
    keys_construct(&self->base.keys);
    /*add keys and check returned key indexes that are the same as expected*/
    int key_index;
    /*check parameters*/
    key_index = self->base.keys.add_key(&self->base.keys, FSTAB_PARAM_CHANNEL_KEY, NULL);
    assert(FSTAB_PARAM_CHANNEL_KEY_INDEX==key_index);
    key_index = self->base.keys.add_key(&self->base.keys, FSTAB_PARAM_MOUNTPOINT_KEY, NULL);
    assert(FSTAB_PARAM_MOUNTPOINT_KEY_INDEX==key_index);
    key_index = self->base.keys.add_key(&self->base.keys, FSTAB_PARAM_ACCESS_KEY, NULL);
    assert(FSTAB_PARAM_ACCESS_KEY_INDEX==key_index);
    key_index = self->base.keys.add_key(&self->base.keys, FSTAB_PARAM_REMOVABLE, FSTAB_VAL_REMOVABLE_NO);
    assert(FSTAB_PARAM_REMOVABLE_KEY_INDEX==key_index);
    key_index = self->base.keys.add_key(&self->base.keys, FSTAB_PARAM_FSNAME_KEY, FSTAB_VAL_FSNAME_DEFAULT);
    assert(FSTAB_PARAM_FSNAME_KEY_INDEX==key_index);

    /*setup functions*/
    s_fstab_observer.base.handle_nvram_record = handle_fstab_record;
    s_fstab_observer.base.is_valid_record = handle_is_valid_record;
    s_fstab_observer.mount_export = handle_mount_export;
    s_fstab_observer.mount_import = handle_mount_import;
    s_fstab_observer.erase_old_mounts = handle_erase_old_mounts;
    s_fstab_observer.reset_removable = handle_reset_removable;
    s_fstab_observer.locate_postpone_mount = handle_locate_postpone_mount;
    s_fstab_observer.postpone_mounts_array = NULL;
    s_fstab_observer.postpone_mounts_count = 0;
    ZRT_LOG(L_SHORT, "OK observer for section: %s", FSTAB_SECTION_NAME);
    s_inited_observer = &s_fstab_observer;
    return s_inited_observer;
}

