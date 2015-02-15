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

#define MOUNT_ITEMS (&s_mounts_manager.mount_items)

int mm_mount_add( const char* path, struct MountsPublicInterface* filesystem_mount );
int mm_fusemount_add( const char* path, struct fuse_operations* fuse_mount );
int mm_mount_remove( const char* path );
struct MountInfo* mm_mountinfo_bypath( const char* path, int *mount_index );
struct MountsPublicInterface* mm_mount_bypath( const char* path);
struct MountsPublicInterface* mm_mount_byhandle( int handle );
const char* mm_convert_path_to_mount(const char* full_path);

static struct MountsManager s_mounts_manager = {
        mm_mount_add,
	mm_fusemount_add,
        mm_mount_remove,
        mm_mountinfo_bypath,
        mm_mount_bypath,
        mm_mount_byhandle,
        mm_convert_path_to_mount,
        NULL,
	NULL,
        NULL
    };


int mm_mount_add( const char* path, struct MountsPublicInterface* filesystem_mount ){
    if ( MOUNT_ITEMS->ptr_array == NULL )
        DynArrayCtor( MOUNT_ITEMS, 2 /*granularity*/ );

    /*check if the same mountpoint haven't used by another mount*/
    int located_index;
    struct MountInfo* existing_mount = mm_mountinfo_bypath( path, &located_index );
    if ( existing_mount && !strcmp(existing_mount->mount_path, path) ){
	SET_ERRNO(EBUSY);
	return -1;
    }
    (void)located_index;
    /*create mount*/
    struct MountInfo *new_mount_info = malloc(sizeof(struct MountInfo));
    new_mount_info->mount_path = strdup(path);
    new_mount_info->mount = filesystem_mount;
    /*add mount*/    
    return ! DynArraySet( MOUNT_ITEMS, MOUNT_ITEMS->num_entries, new_mount_info );
}


int mm_fusemount_add( const char* path, struct fuse_operations* fuse_mount ){
    struct MountsPublicInterface* fs 
	= CONSTRUCT_L(FUSE_OPERATIONS_MOUNT)( get_handle_allocator(),
					      get_open_files_pool(),
					      fuse_mount );
    return mm_mount_add( path, fs );
}

int mm_mount_remove( const char* path ){
    int located_index;
    struct MountInfo* mount = mm_mountinfo_bypath( path, &located_index );
    if (mount!=NULL){
        free(mount->mount);
        free(mount->mount_path);
        free(mount);
        if ( DynArraySet(MOUNT_ITEMS, located_index, NULL) != 0 )
            return 0;
    }
    return -1; /*todo implement it*/
}


struct MountInfo* mm_mountinfo_bypath( const char* path, int *mount_index ){
    struct MountInfo *mount_info=NULL;
    int i;
    for( i=0; i < MOUNT_ITEMS->num_entries; i++ ){
        /*if matched path and mount path*/
        struct MountInfo *current_mount_info = (struct MountInfo *)DynArrayGet(MOUNT_ITEMS, i);
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


struct MountsPublicInterface* mm_mount_bypath( const char* path){
    int located_mount_index;
    struct MountInfo* mount_info = mm_mountinfo_bypath(path, &located_mount_index);
    (void)located_mount_index;
    if ( mount_info )
        return mount_info->mount;
    else
        return NULL;
}

struct MountsPublicInterface* mm_mount_byhandle( int handle ){
    const struct HandleItem* entry = s_mounts_manager.handle_allocator->entry(handle);
    /*if handle exist and related to opened file*/
    if ( entry && s_mounts_manager.open_files_pool
	 ->entry(entry->open_file_description_id) != NULL ) 
	return entry->mount_fs;
    else return NULL;
}

const char* mm_convert_path_to_mount(const char* full_path){
    int located_mount_index;
    struct MountInfo* mount_info = mm_mountinfo_bypath( full_path, &located_mount_index );
    if ( mount_info ){
	if ( mount_info->mount->mount_id == EChannelsMountId ){
	    /*for channels mount do not use path transformation*/
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

struct MountsManager* mounts_manager(){
    return &s_mounts_manager;    
}

struct MountsManager* get_mounts_manager(){
    s_mounts_manager.handle_allocator = get_handle_allocator();
    s_mounts_manager.open_files_pool = get_open_files_pool();
    return &s_mounts_manager;
}


