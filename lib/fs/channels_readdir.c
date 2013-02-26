/*
 * readdir.c
 *
 *  Created on: 03.08.2012
 *      Author: yaroslav
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

#include "zvm.h"
#include "channels_readdir.h"
#include "zrtlog.h"
#include "nacl_struct.h"
#include "channels_mount.h"


struct dir_data_t *
match_dir_in_directory_list(struct manifest_loaded_directories_t *manifest_dirs, const char *dirpath, int len){
    assert(manifest_dirs);
    struct dir_data_t *dir = NULL;
    int i;
    for( i=0; i < manifest_dirs->dircount; i++ ){
        dir = &manifest_dirs->dir_array[i];
        if ( ! strncmp(dirpath, dir->path, len ) && strlen(dir->path) == len ) return dir;
    }
    return NULL;
}

struct dir_data_t *
match_handle_in_directory_list(struct manifest_loaded_directories_t *manifest_dirs, int handle){
    assert(manifest_dirs);
    struct dir_data_t *dir = NULL;
    int i;
    for( i=0; i < manifest_dirs->dircount; i++ ){
        dir = &manifest_dirs->dir_array[i];
        if ( dir->handle == handle ) return dir;
    }
    return NULL;
}

int callback_add_dir(struct manifest_loaded_directories_t *manifest_dirs, const char *dirpath, int len){
    assert(manifest_dirs);
    if ( !len ) return -1;
    const struct dir_data_t * found = match_dir_in_directory_list( manifest_dirs, dirpath, len);
    /*if dir path not found then add it*/
    if ( !found ){
        /*if directory can be added array size not exceeded*/
        if ( manifest_dirs->dircount < DIRECTORIES_MAX_NUMBER ){
            /*handle will assigned from 0 now, but should be increased on channels number, to be unique
             *nlink assigned now 2, should be increased on subdirs number*/
            struct dir_data_t *d = &manifest_dirs->dir_array[manifest_dirs->dircount];
            d->handle = manifest_dirs->dircount;
            d->nlink =2;
            d->path = calloc(sizeof(char), len+1);
            memcpy( d->path, dirpath, len );
            manifest_dirs->dircount++;
            return 0;
        }
        else{
            /*can't append dir to array : directories count should not exceed */
            return -2;
        }
    }
    else
        return -1;
}

/*Search rightmost '/' and return substring*/
void process_subdirs_via_callback(
        int (*callback_add_dir)(struct manifest_loaded_directories_t *, const char *, int), /*callback*/
        struct manifest_loaded_directories_t *manifest_dirs,
        const char *path, int len ){
    /*locate rightmost '/' inside path
     * can't use strrchr for char array path with len=length because path[len] not null terminated char*/
    const char *dirpath = NULL;
    int i;
    for(i=len-1; i >=0; i--){
        if(path[i] == '/'){
            dirpath = &path[i];
            break;
        }
    }
    if ( dirpath ){
        int subpathlen = dirpath-path;
        /*if matched root path '/' it should be processed as '/'*/
        if ( dirpath[0]=='/' && len > 1 && !subpathlen ){
            subpathlen = 1;
        }
        process_subdirs_via_callback( callback_add_dir, manifest_dirs, path, subpathlen );
        (*callback_add_dir)(manifest_dirs, path, subpathlen);
    }
    return;
}


const char* name_from_path_get_path_len(const char *fullpath, int *pathlen){
    /*locate most right dir by symbol '/'*/
    char *c = strrchr(fullpath, '/');
    if ( c && *++c != '\0' ){
        *pathlen = (c - fullpath) / sizeof(char);
        /*paths should not include trailing '/' symbol, for example should be '/dev'
         *Exclusion is a root path '/' */
        if ( *pathlen > 1 )
            --*pathlen;
        return (const char*)( c );
    }
    else return 0;
}


void process_channels_create_dir_list( const struct ZVMChannel *channels, int channels_count,
        struct manifest_loaded_directories_t *manifest_dirs )
{
    int i;
    for( i=0; i < channels_count; i++ ){
        /*process full path name including all sub dirs*/
        process_subdirs_via_callback(callback_add_dir, manifest_dirs, channels[i].name, strlen(channels[i].name));
    }

    int index;
    int subdirs_count;
    for ( i=0; i < manifest_dirs->dircount; i++ ){
        /*calculate subdirs count*/
        subdirs_count = 0;
        index = 0; /*start search starting from 0-item of dirs array*/
        do{
            index = get_sub_dir_index( manifest_dirs, manifest_dirs->dir_array[i].path, index );
            if ( index != -1 ){
                subdirs_count++;
            }
        }while( index++ != -1 );
        /*calculate ok*/

        /*do unique handles for all manifest parsed directories*/
        manifest_dirs->dir_array[i].handle += channels_count;
        /*get subdirs count, update nlink*/
        manifest_dirs->dir_array[i].nlink += subdirs_count;
    }
}


/* To search many subdirs user would call it with index parameter returned by previous
 * function call; search subdir starting from index in manifest_dirs, if matched return 
 * subdir index related to manifest_dirs;
 * @return subdir index if found, -1 if not*/
int get_sub_dir_index( struct manifest_loaded_directories_t *manifest_dirs, const char *dirpath, int index )
{
    struct dir_data_t * d = NULL;
    int i;
    int namelen =  strlen(dirpath);
    for( i=index; i < manifest_dirs->dircount; i++ ){
        d = &manifest_dirs->dir_array[i];
        /*match all subsets, exclude the same dir path*/
        if ( ! strncmp(dirpath, d->path, namelen) && strlen(d->path) > namelen+1 ){
            /*if can't locate trailing '/' then matched subdir*/
            char *backslash_matched = strchr( &d->path[namelen+1], '/');
            if ( !backslash_matched ){
                return i;
            }
        }
    }
    return -1;
}


size_t put_dirent_into_buf( char *buf, int buf_size, unsigned long d_ino, unsigned long d_off,
        const char *d_name, int namelength ){
    #define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
    struct nacl_abi_dirent *dirent = (struct nacl_abi_dirent *) buf;
    ZRT_LOG(L_EXTRA, "dirent offset: ino_off=%u, off_off=%u, reclen_off=%u, name_off=%u",
            offsetof(struct nacl_abi_dirent, d_ino ),
            offsetof(struct nacl_abi_dirent, d_off ),
            offsetof(struct nacl_abi_dirent, d_reclen ),
            offsetof(struct nacl_abi_dirent, d_name ) );

    /*dirent structure not have constant size it's can be vary depends on name length.       
      also dirent size should be multiple of the 8 bytes, so adjust it*/
    uint32_t adjusted_size = 
	offsetof(struct nacl_abi_dirent, d_name) + namelength +1 /* NUL termination */;
    adjusted_size = ROUND_UP( adjusted_size, 8 );

    /*if size of the current dirent data is less than available buffer size
     then fill it by data*/
    if ( adjusted_size < buf_size ){
        dirent->d_reclen = ROUND_UP( adjusted_size, 8 );
        dirent->d_ino = d_ino;
        if ( d_off == 0x7fffffff )
            dirent->d_off = 0x7fffffff;
        else
            dirent->d_off = d_off+dirent->d_reclen;

        memcpy( dirent->d_name, d_name, namelength );
        ((char*)dirent->d_name)[namelength] = '\0';

        ZRT_LOG(L_SHORT, "dirent: name=%s, ino=%u, d_off=%u, d_reclen=%d, namel=%d",
                d_name, 
		(unsigned int)d_ino, 
		(unsigned int)d_off, 
		dirent->d_reclen, 
		namelength );
        return dirent->d_reclen;
    }
    /*buffer is not enough to save current dirent structure*/
    else{
        ZRT_LOG(L_EXTRA, "no enough buffer, "
		"data_size=%d, buf_size=%d", 
		adjusted_size, buf_size);
        return -1; /*no enough buffer size*/
    }
}
