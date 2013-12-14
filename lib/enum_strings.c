/*
 * enum_strings.c
 * string constants, for debugging purposes
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


#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <alloca.h>

#include "unpack_interface.h"
#include "enum_strings.h"
#include "zrt_helper_macros.h"
#include "zrtlog.h"

static char s_buffer[MAX_FLAGS_LEN];

/****************************************************************
 *flags count should be equal to flag texts count for every pair
 *of flags and texts 
 ****************************************************************/

struct enum_data_t{
    int  flag;
    char* text;
};

#define EITEM(item) {item, #item}

static struct enum_data_t s_file_open_array0[] = {
    EITEM(O_CREAT), EITEM(O_RDONLY), EITEM(O_WRONLY), EITEM(O_RDWR),
    EITEM(O_EXCL), EITEM(O_TRUNC), EITEM(O_DIRECT), 
    EITEM(O_DIRECTORY),EITEM(O_NOATIME), EITEM(O_APPEND), EITEM(O_ASYNC), 
    EITEM(O_SYNC), EITEM(O_NONBLOCK), EITEM(O_NDELAY), EITEM(O_NOCTTY)
};

static struct enum_data_t s_mmap_prot_array1[] = {
    EITEM(PROT_EXEC), EITEM(PROT_READ), EITEM(PROT_WRITE), EITEM(PROT_NONE) 
};

static struct enum_data_t s_mmap_flags_array2[] = {
    EITEM(MAP_SHARED), EITEM(MAP_PRIVATE), EITEM(MAP_32BIT), EITEM(MAP_ANON),
    EITEM(MAP_ANONYMOUS), EITEM(MAP_DENYWRITE), EITEM(MAP_EXECUTABLE),
    EITEM(MAP_FILE), EITEM(MAP_FIXED), EITEM(MAP_GROWSDOWN), EITEM(MAP_LOCKED),
    EITEM(MAP_NONBLOCK), EITEM(MAP_NORESERVE), EITEM(MAP_POPULATE), EITEM(MAP_STACK)
};

static struct enum_data_t s_seek_whence_array3[] = {
    EITEM(SEEK_SET), EITEM(SEEK_CUR), EITEM(SEEK_END)
};

static struct enum_data_t s_lock_type_array4[] = {
    EITEM(F_RDLCK), EITEM(F_WRLCK), EITEM(F_UNLCK)
};

static struct enum_data_t s_fcntl_cmd_array5[] = {
    EITEM(F_DUPFD), EITEM(F_GETFD), EITEM(F_SETFD), EITEM(F_GETFL), 
    EITEM(F_SETFL), EITEM(F_GETLK), EITEM(F_SETLK), EITEM(F_SETLKW)
};

static struct enum_data_t s_fileopen_mode_array6[] = {
    EITEM(S_IXOTH), EITEM(S_IWOTH), EITEM(S_IROTH), EITEM(S_IRWXO),
    EITEM(S_IXGRP), EITEM(S_IWGRP), EITEM(S_IRGRP), EITEM(S_IRWXG), 
    EITEM(S_IXUSR), EITEM(S_IWUSR), EITEM(S_IRUSR), EITEM(S_IRWXU)
};

static struct enum_data_t s_archive_entry_type_array7[] = {
    EITEM(ETypeFile), EITEM(ETypeDir)
};

static struct enum_data_t s_stat_mode_array8[] = {
    EITEM(S_IFMT), EITEM(S_IFSOCK), EITEM(S_IFLNK), EITEM(S_IFREG), EITEM(S_IFBLK),
    EITEM(S_IFDIR),EITEM(S_IFCHR),  EITEM(S_IFIFO), EITEM(S_ISUID), EITEM(S_ISGID),
    EITEM(S_ISVTX),EITEM(S_IRWXU),  EITEM(S_IRUSR), EITEM(S_IWUSR), EITEM(S_IXUSR),
    EITEM(S_IRWXG),EITEM(S_IRGRP),  EITEM(S_IWGRP), EITEM(S_IXGRP), EITEM(S_IRWXO),
    EITEM(S_IROTH),EITEM(S_IWOTH),  EITEM(S_IXOTH)
};

/*add here new arrays*/
static struct enum_data_t* array_by_enum(int index, int* size){
#define GET_SIZE_ARRAY(arr) sizeof(arr)/sizeof(struct enum_data_t)
    switch(index){
    case EFileOpenFlags:
	*size = GET_SIZE_ARRAY(s_file_open_array0);
	return s_file_open_array0;
    case EMmapProtFlags:
	*size = GET_SIZE_ARRAY(s_mmap_prot_array1);
	return s_mmap_prot_array1;
    case EMMapFlags:
	*size = GET_SIZE_ARRAY(s_mmap_flags_array2);
	return s_mmap_flags_array2;
    case ESeekWhence:
	*size = GET_SIZE_ARRAY(s_seek_whence_array3);
	return s_seek_whence_array3;
    case ELockTypeFlags:
	*size = GET_SIZE_ARRAY(s_lock_type_array4);
	return s_lock_type_array4;
    case EFcntlCmd:
	*size = GET_SIZE_ARRAY(s_fcntl_cmd_array5);
	return s_fcntl_cmd_array5;
    case EFileOpenMode:
	*size = GET_SIZE_ARRAY(s_fileopen_mode_array6);
	return s_fileopen_mode_array6;
    case EArchEntryType:
	*size = GET_SIZE_ARRAY(s_archive_entry_type_array7);
	return s_archive_entry_type_array7;
    case EStatStMode:
	*size = GET_SIZE_ARRAY(s_stat_mode_array8);
	return s_stat_mode_array8;
    default:
	assert(0);
	break;
    }
}

/*=================implementation===============*/

const char* text_from_flag( int flags, enum array_list_t enum_id ){
    memset(s_buffer, '\0', MAX_FLAGS_LEN );
    int array_len = 0;
    const struct enum_data_t* array = array_by_enum(enum_id, &array_len);

    int output_space=0;
    /*empty_flag_index is used to save empty flag and after flags processing not found
     * setted flags then output flag at index; */
    int empty_flag_index =-1; 
    int i;
    char delim_char = '|';

    for ( i=0; i < array_len; i++ ){
	/*save empty flag index*/
	if ( array[i].flag == 0 ) empty_flag_index = i; 
	/*check every flag and if flag is set add it to s buffer for further logging*/
        if ( CHECK_FLAG(flags, array[i].flag) && array[i].flag != 0 ){
	    if ( output_space > 0 ) delim_char = '|';
	    else delim_char = ' ';
	    output_space += snprintf( s_buffer+output_space, MAX_FLAGS_LEN-output_space, 
				      "%c%s", delim_char, array[i].text );
	    if ( output_space > MAX_FLAGS_LEN ){
		ZRT_LOG(L_ERROR, 
			"Flags output truncated, insufficient buffer size, required%d.", 
			output_space);
		break;
	    }
        }
    }
    /*display empty flag if not found setted flags*/
    if ( output_space == 0 ){
	if ( empty_flag_index != -1 ){
	    strcpy(s_buffer, array[empty_flag_index].text);
	}
	else{
	    strcpy(s_buffer, "none");
	}
    }

    return s_buffer;
}

/*=================implementation===============*/

const char* text_from_id( int id, enum array_list_t enum_id ){
    memset(s_buffer, '\0', MAX_FLAGS_LEN );
    int array_len = 0;
    const struct enum_data_t* array = array_by_enum(enum_id, &array_len);

    int output_space=0;
    int i;

    for ( i=0; i < array_len; i++ ){
	/*check every flag and if flag is set add it to s buffer for further logging*/
        if ( id == array[i].flag ){
	    output_space += snprintf( s_buffer, MAX_FLAGS_LEN, 
				      "%s", array[i].text );
	    break;
        }
    }
    /*display that id as unknown*/
    if ( output_space == 0 ){
	snprintf( s_buffer, MAX_FLAGS_LEN, "%d unknown", id );
    }
    return s_buffer;
}

