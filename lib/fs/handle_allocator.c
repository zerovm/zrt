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

#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#include "zrtlog.h"
#include "handle_allocator.h"

#define VERIFY_HANDLE(handle, state)					\
    ((handle<0 || handle>=MAX_HANDLES_COUNT)? 0 : (s_handle_slots[handle].used == state)) 


enum { EHandleAvailable=0, EHandleUsed=1  };

struct HandleItemInternal{
    struct HandleItem public_;
    int used; /*EHandleAvailable, EHandleUsed*/
};



static int s_first_unused_slot = 0;
static struct HandleItemInternal s_handle_slots[MAX_HANDLES_COUNT];

static int seek_unused_slot( int starting_from ){
    int i;
    starting_from = starting_from<0 ?0 :starting_from;
    for ( i=starting_from; i < MAX_HANDLES_COUNT; i++ ){
        if ( s_handle_slots[i].used == EHandleAvailable ){
	    /*found closest unusable slot*/
            ZRT_LOG( L_INFO, "slot index=%d, EHandleAvailable", i );
            s_first_unused_slot = i;
            return s_first_unused_slot;
        }
    }
    return -1;
}


static int allocate_handle(struct MountsPublicInterface* mount_fs, 
			   ino_t inode, 
			   ino_t parent_dir_inode,
			   int open_file_desc_id){
    s_first_unused_slot = seek_unused_slot( s_first_unused_slot );
    if ( s_first_unused_slot == -1 ) return -1;
    s_handle_slots[s_first_unused_slot].used = EHandleUsed;
    s_handle_slots[s_first_unused_slot].public_.mount_fs = mount_fs;
    s_handle_slots[s_first_unused_slot].public_.open_file_description_id = open_file_desc_id;
    s_handle_slots[s_first_unused_slot].public_.inode = inode;
    s_handle_slots[s_first_unused_slot].public_.parent_dir_inode = parent_dir_inode;
    return s_first_unused_slot;
}

static int allocate_handle2(struct MountsPublicInterface* mount_fs, 
			    ino_t inode, 
			    ino_t parent_dir_inode,
			    int open_file_desc_id, 
			    int handle){
    if ( !VERIFY_HANDLE(handle, EHandleAvailable) ) return -1;
    s_handle_slots[handle].used = EHandleUsed;
    s_handle_slots[handle].public_.mount_fs = mount_fs;
    s_handle_slots[handle].public_.open_file_description_id = open_file_desc_id;
    s_handle_slots[handle].public_.inode = inode;
    s_handle_slots[handle].public_.parent_dir_inode = parent_dir_inode;
    return handle;
}

static int free_handle(int handle){
    if ( !VERIFY_HANDLE(handle, EHandleUsed) ) return -1;
    s_handle_slots[handle].used = EHandleAvailable;
    s_handle_slots[handle].public_.mount_fs = NULL;
    s_handle_slots[handle].public_.open_file_description_id = 0;
    s_handle_slots[handle].public_.inode = 0;
    /*set lowest available slot*/
    if ( handle < s_first_unused_slot ) s_first_unused_slot = handle;
    return 0; //ok

}

static int check_handle_is_related_to_filesystem(int handle, struct MountsPublicInterface* fs){
    if ( !VERIFY_HANDLE(handle, EHandleUsed) ) return -1;
    if ( s_handle_slots[handle].public_.mount_fs == fs )
        return 0; //ok
    else
	return -1;
}

struct MountsPublicInterface* mount_interface(int handle){
    if ( !VERIFY_HANDLE(handle, EHandleUsed) ) return NULL;
    return s_handle_slots[handle].public_.mount_fs;
}

static const struct HandleItem* entry(int handle){
    if ( !VERIFY_HANDLE(handle, EHandleUsed) ) return NULL;
    return &s_handle_slots[handle].public_;
}

static const struct OpenFileDescription* ofd(int handle){
    if ( !VERIFY_HANDLE(handle, EHandleUsed) ) return NULL;
    return get_open_files_pool()->entry( s_handle_slots[handle].public_.open_file_description_id );
}


static struct HandleAllocator s_handle_allocator = {
    allocate_handle,
    allocate_handle2,
    free_handle,
    check_handle_is_related_to_filesystem,
    mount_interface,
    entry,
    ofd
};


struct HandleAllocator* get_handle_allocator(){
    return &s_handle_allocator;
}

