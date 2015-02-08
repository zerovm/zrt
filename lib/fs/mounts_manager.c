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
#include <linux/limits.h>
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"
#include "zrt_helper_macros.h" //SET_ERRNO
#include "mounts_manager.h"
#include "mounts_interface.h"
#include "fuse_operations_mount.h"
#include "handle_allocator.h"
#include "open_file_description.h"

#ifndef MIN
#  define MIN(a,b)( a<b?a:b )
#endif

int mm_mount_add( const char* path, struct MountsPublicInterface* filesystem_mount );
int mm_fusemount_add( const char* path, struct fuse_operations* fuse_mount );
int mm_mount_remove( const char* path );
struct MountInfo* mm_mountinfo_bypath( const char* path );
struct MountsPublicInterface* mm_mount_bypath( const char* path );
struct MountsPublicInterface* mm_mount_byhandle( int handle );
const char* mm_convert_path_to_mount(const char* full_path);

static int s_mount_items_count;
static struct MountInfo s_mount_items[EMountsCount];
static struct MountsManager s_mounts_manager = {
        mm_mount_add,
	mm_fusemount_add,
        mm_mount_remove,
        mm_mountinfo_bypath,
        mm_mount_bypath,
        mm_mount_byhandle,
        mm_convert_path_to_mount,
        NULL,
	NULL
    };


int mm_mount_add( const char* path, struct MountsPublicInterface* filesystem_mount ){
    assert( s_mount_items_count < EMountsCount );
    /*if no empty slots*/
    if ( s_mount_items_count >= EMountsCount ){
	SET_ERRNO(ENOTEMPTY);
	return -1; /*no empty slots*/
    }
    /*check if the same mountpoint haven't used by another mount*/
    struct MountInfo* existing_mount = mm_mountinfo_bypath( path );
    if ( existing_mount && !strcmp(existing_mount->mount_path, path) ){
	SET_ERRNO(EBUSY);
	return -1;
    }
    /*add mount*/
    memcpy( s_mount_items[s_mount_items_count].mount_path, path, MIN( strlen(path), PATH_MAX ) );
    s_mount_items[s_mount_items_count].mount = filesystem_mount;
    ++s_mount_items_count;
    return 0;
}


int mm_fusemount_add( const char* path, struct fuse_operations* fuse_mount ){
    struct MountsPublicInterface* fs 
	= CONSTRUCT_L(FUSE_OPERATIONS_MOUNT)( get_handle_allocator(),
					      get_open_files_pool(),
					      fuse_mount );
    return mm_mount_add( path, fs );
}

int mm_mount_remove( const char* path ){
    return -1; /*todo implement it*/
}


struct MountInfo* mm_mountinfo_bypath( const char* path ){
    int most_long_mount=-1;
    int i;
    for( i=0; i < s_mount_items_count; i++ ){
        /*if matched path and mount path*/
        const char* mount_path = s_mount_items[i].mount_path;
        if ( !strncmp( mount_path, path, strlen(mount_path) ) ){
            if ( most_long_mount == -1 || 
		 strlen(mount_path) > strlen(s_mount_items[most_long_mount].mount_path) ){
                most_long_mount=i;
            }
        }
    }

    if ( most_long_mount != -1 ){
	ZRT_LOG(L_EXTRA, "mounted_on_path=%s", 
		s_mount_items[most_long_mount].mount_path);
        return &s_mount_items[most_long_mount];
    }
    else
        return NULL;
}


struct MountsPublicInterface* mm_mount_bypath( const char* path ){
    struct MountInfo* mount_info = mm_mountinfo_bypath(path);
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
    struct MountInfo* mount_info = mm_mountinfo_bypath( full_path );
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


