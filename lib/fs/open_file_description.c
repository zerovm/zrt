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

#include "open_file_description.h"
#include "handle_allocator.h" //MAX_HANDLES_COUNT


#define VERIFY_OFD(id)							\
    ((id<0 || id>=MAX_HANDLES_COUNT)? 0					\
     : (s_open_files_array[id].refcount>0)) 


struct OpenFileDescInternal{
    struct OpenFileDescription public_;
    int refcount;
};

static int s_first_unused_slot=0;
static struct OpenFileDescInternal s_open_files_array[MAX_HANDLES_COUNT];

static int seek_unused_slot( int starting_from ){
    int i;
    starting_from = starting_from<0 ?0 :starting_from;
    for ( i=starting_from; i < MAX_HANDLES_COUNT; i++ ){
        if ( s_open_files_array[i].refcount == 0 ){
            s_first_unused_slot = i;
            return s_first_unused_slot;
        }
    }
    return -1;
}


static int getnew_ofd(int flags){
    s_first_unused_slot = seek_unused_slot( s_first_unused_slot );
    if ( s_first_unused_slot == -1 ) return -1;
    ++s_open_files_array[s_first_unused_slot].refcount;
    s_open_files_array[s_first_unused_slot].public_.offset=0;
    s_open_files_array[s_first_unused_slot].public_.channel_sequential_offset=0;
    s_open_files_array[s_first_unused_slot].public_.flags=flags;
    return s_first_unused_slot;
}


static int refer_ofd(int id_ofd){
    if ( !VERIFY_OFD(id_ofd) ) return -1;
    ++s_open_files_array[id_ofd].refcount;
    return 0;
}

static int release_ofd(int id_ofd){
    if ( !VERIFY_OFD(id_ofd) ) return -1;
    if ( !--s_open_files_array[id_ofd].refcount ){
	/*set lowest available slot*/
	if ( id_ofd < s_first_unused_slot ) s_first_unused_slot = id_ofd;
    }
    return 0;
}

static const struct OpenFileDescription* entry(int id_ofd){
    if ( !VERIFY_OFD(id_ofd) ) return NULL;
    return &s_open_files_array[id_ofd].public_;
}


static int set_offset_sequential_channel(int id_ofd, off_t offset ){
    if ( !VERIFY_OFD(id_ofd) ) return -1;
    s_open_files_array[id_ofd].public_.channel_sequential_offset = offset;
    return 0;
}


static int set_offset(int id_ofd, off_t offset ){
    if ( !VERIFY_OFD(id_ofd) ) return -1;
    s_open_files_array[id_ofd].public_.offset = offset;
    return 0;
}


static int set_flags(int id_ofd, int flags ){
    if ( !VERIFY_OFD(id_ofd) ) return -1;
    s_open_files_array[id_ofd].public_.flags = flags;
    return 0;
}


static struct OpenFilesPool s_open_files_pool = {
    getnew_ofd,
    refer_ofd,
    release_ofd,    
    entry,
    set_offset_sequential_channel,
    set_offset,
    set_flags
};


struct OpenFilesPool* get_open_files_pool(){
    return &s_open_files_pool;
}

