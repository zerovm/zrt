/*
 * mounts_manager.c
 *
 *  Created on: 19.09.2012
 *      Author: yaroslav
 */

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <linux/limits.h>
#include <assert.h>

#include "zrtlog.h"
#include "mounts_manager.h"
#include "mounts_interface.h"
#include "handle_allocator.h"

#define MIN(a,b)( a<b?a:b )

int mm_mount_add( const char* path, struct MountsInterface* filesystem_mount );
int mm_mount_remove( const char* path );
struct MountInfo* mm_mountinfo_bypath( const char* path );
struct MountsInterface* mm_mount_bypath( const char* path );
struct MountsInterface* mm_mount_byhandle( int handle );
const char* mm_get_nested_mount_path(struct MountInfo* mount_info, const char* full_path);

static int s_mount_items_count;
static struct MountInfo s_mount_items[EMountsCount];
static struct MountsManager s_mounts_manager = {
        mm_mount_add,
        mm_mount_remove,
        mm_mountinfo_bypath,
        mm_mount_bypath,
        mm_mount_byhandle,
        mm_get_nested_mount_path,
        NULL
    };


int mm_mount_add( const char* path, struct MountsInterface* filesystem_mount ){
    assert( s_mount_items_count < EMountsCount );
    memcpy( s_mount_items[s_mount_items_count].mount_path, path, MIN( strlen(path), PATH_MAX ) );
    s_mount_items[s_mount_items_count].mount = filesystem_mount;
    ++s_mount_items_count;
    return 0;
}

int mm_mount_remove( const char* path ){
    return -1; /*todo implement it*/
}


struct MountInfo* mm_mountinfo_bypath( const char* path ){
    zrt_log("path=%s", path);
    int most_long_mount=-1;
    int i;
    for( i=0; i < s_mount_items_count; i++ ){
        /*if matched path and mount path*/
        const char* mount_path = s_mount_items[i].mount_path;
        zrt_log("mount_path=%s", mount_path);
        if ( !strncmp( mount_path, path, strlen(mount_path) ) ){
            if ( most_long_mount == -1 || strlen(mount_path) > strlen(s_mount_items[most_long_mount].mount_path) ){
                most_long_mount=i;
            }
        }
    }

    if ( most_long_mount != -1 )
        return &s_mount_items[most_long_mount];
    else
        return NULL;
}


struct MountsInterface* mm_mount_bypath( const char* path ){
    struct MountInfo* mount_info = mm_mountinfo_bypath(path);

    if ( mount_info )
        return mount_info->mount;
    else
        return NULL;
}

struct MountsInterface* mm_mount_byhandle( int handle ){
    return s_mounts_manager.handle_allocator->mount_interface(handle);
}

const char* mm_get_nested_mount_path(struct MountInfo* mount_info, const char* full_path){
    if ( strlen(mount_info->mount_path) == 1 && mount_info->mount_path[0] == '/' )
        return full_path; /*use paths mounted on root '/' as is*/
    else{
        /*get path relative to mount path.
         * for example: full_path="/tmp/fire", mount_path="/tmp", returned="/fire"  */
        return &full_path[ strlen( mount_info->mount_path ) ];
    }
}


struct MountsManager* alloc_mounts_manager(){
    s_mounts_manager.handle_allocator = alloc_handle_allocator();
    return &s_mounts_manager;
}


