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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h" //SET_ERRNO
#include "mounts_manager.h"
#include "mounts_interface.h"
#include "fs/fuse/fuse_operations_mount.h"
#include "handle_allocator.h"
#include "open_file_description.h"
#include "dyn_array.h"

#ifndef MIN
#  define MIN(a,b)( a<b?a:b )
#endif

struct MountInfo* mm_mountinfo_bypath( struct MountsManager *mounts_manager,
                                       const char* path, int *mount_index ){
    struct MountInfo *mount_info=NULL;
    int i;
    for( i=0; i < mounts_manager->mount_items.num_entries; i++ ){
        /*if matched path and mount path*/
        struct MountInfo *current_mount_info 
            = (struct MountInfo *)DynArrayGet(&mounts_manager->mount_items, i);
        if ( current_mount_info == NULL ) continue;
        const char* mount_path = current_mount_info->mount_path;
        /*if path is matched, then check mountpoint*/
        if ( !strncmp( mount_path, path, strlen(mount_path) ) ){
            if ( mount_info == NULL ||
                 strlen(mount_path) > strlen(mount_info->mount_path) ){
                mount_info = current_mount_info;
                *mount_index=i;
            }
        }
    }

    if ( mount_info != NULL ){
	ZRT_LOG(L_EXTRA, "located mount by path=%s: mountpoint=%s, mount_index=%d", 
                path, mount_info->mount_path, *mount_index);
        return mount_info;
    }
    else{
	ZRT_LOG(L_EXTRA, "didn't locate mount by path=%s", path);
        return NULL;
    }
}

int mm_mount_add( struct MountsManager *mounts_manager,
                  const char* path, 
                  struct MountsPublicInterface* filesystem_mount,
                  char expect_absolute_path){
    /*check if the same mountpoint haven't used by another mount*/
    int located_index;
    struct MountInfo* existing_mount = mm_mountinfo_bypath( mounts_manager, path, &located_index );
    if ( existing_mount && !strcmp(existing_mount->mount_path, path) ){
	SET_ERRNO(EBUSY);
	return -1;
    }
    (void)located_index;
    /*create mount*/
    struct MountInfo *new_mount_info = malloc(sizeof(struct MountInfo));
    new_mount_info->mount_path = strdup(path);
    new_mount_info->expect_absolute_path = expect_absolute_path;
    new_mount_info->mount = filesystem_mount;
    /*add mount*/    
    return ! DynArraySet( &mounts_manager->mount_items, 
                          mounts_manager->mount_items.num_entries, new_mount_info );
}


int mm_fusemount_add( struct MountsManager *mounts_manager,
                      const char* path, 
                      struct fuse_operations* fuse_mount,
                      char expect_absolute_path,
                      char proxy_mode){
    struct MountsPublicInterface* fs 
	= CONSTRUCT_L(FUSE_OPERATIONS_MOUNT)( get_handle_allocator(),
					      get_open_files_pool(),
					      fuse_mount,
                                              proxy_mode);
    return mm_mount_add( mounts_manager, path, fs, expect_absolute_path );
}

int mm_mount_remove( struct MountsManager *mounts_manager,
                     const char* path ){
    int located_index;
    struct MountInfo* mount = mm_mountinfo_bypath( mounts_manager, path, &located_index );
    if (mount!=NULL){
        free(mount->mount);
        free(mount->mount_path);
        free(mount);
        if ( DynArraySet(&mounts_manager->mount_items, located_index, NULL) != 0 )
            return 0;
    }
    return -1; /*todo implement it*/
}


struct MountsPublicInterface* mm_mount_bypath( struct MountsManager *mounts_manager,
                                               const char* path){
    int located_mount_index;
    struct MountInfo* mount_info = mm_mountinfo_bypath(mounts_manager, path, &located_mount_index);
    (void)located_mount_index;
    if ( mount_info )
        return mount_info->mount;
    else
        return NULL;
}

struct MountsPublicInterface* mm_mount_byhandle( struct MountsManager *mounts_manager,
                                                 int handle ){
    const struct HandleItem* entry = get_handle_allocator()->entry(handle);
    /*if handle exist and related to opened file*/
    if ( entry && NULL != 
         get_open_files_pool()->entry(entry->open_file_description_id) ) 
	return entry->mount_fs;
    else return NULL;
}

const char* mm_convert_path_to_mount(struct MountsManager *mounts_manager,
                                     const char* full_path){
    int located_mount_index;
    struct MountInfo* mount_info = mm_mountinfo_bypath( mounts_manager, full_path, &located_mount_index );
    if ( mount_info ){
	if ( mount_info->expect_absolute_path == EAbsolutePathExpected ){
	    /*for mounts which do not use path transformation*/
	    return full_path;
	}
	else{
	    if ( strlen(mount_info->mount_path) == 1 && mount_info->mount_path[0] == '/' )
		return full_path; /*use paths mounted on root '/' as is*/
	    else{
		/*get path relative to mount path.
		 * for example: full_path="/tmp/fire", mount_path="/tmp", returned="/fire" */
		return &full_path[ strlen( mount_info->mount_path ) ];
	    }
	}
    }
    else
	return NULL;
}

struct MountsManager* alloc_mounts_manager(){
    struct MountsManager *self = malloc(sizeof(struct MountsManager));
    self->mount_add               = mm_mount_add;
    self->fusemount_add           = mm_fusemount_add;
    self->mount_remove            = mm_mount_remove;
    self->mountinfo_bypath        = mm_mountinfo_bypath;
    self->mount_bypath            = mm_mount_bypath;
    self->mount_byhandle          = mm_mount_byhandle;
    self->convert_path_to_mount   = mm_convert_path_to_mount;
    DynArrayCtor( &self->mount_items, 2 /*granularity*/ );
    return self;
}
