/*
 * readdir.c
 *
 *  Created on: 03.08.2012
 *      Author: yaroslav
 */

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include "zvm.h"
#include "zrtreaddir.h"
#include "zrtsyscalls.h"



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


void process_channels_create_dir_list( struct ZVMChannel *channels, int channels_count,
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


/* To search many subdirs user would call it with index parameter returned by previous function call;
 * search subdir starting from index in manifest_dirs, if matched return matched subdir index
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

int get_dir_content_channel_index( struct ZVMChannel *channels, int channels_count,
        const char *dirpath, int index )
{
    int i;
    int dirlen =  strlen(dirpath);
    for( i=index; i < channels_count; i++ ){
        /*match all subsets for dir path*/
        if ( ! strncmp(dirpath, channels[i].name, dirlen) && strlen(channels[i].name) > dirlen+1 ){
            /*if can't locate trailing '/' then matched directory file*/
            char *backslash_matched = strchr( &channels[i].name[dirlen+1], '/');
            if ( !backslash_matched )
                return i;
        }
    }
    return -1;
}



#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
size_t put_dirent_into_buf( char *buf, int buf_size, unsigned long d_ino, unsigned long d_off,
        const char *d_name, int namelength ){
    struct nacl_abi_dirent *dirent = (struct nacl_abi_dirent *) buf;
    zrt_log( "dirent offset: ino_off=%u, off_off=%u, reclen_off=%u, name_off=%u",
            offsetof(struct nacl_abi_dirent, d_ino ),
            offsetof(struct nacl_abi_dirent, d_off ),
            offsetof(struct nacl_abi_dirent, d_reclen ),
            offsetof(struct nacl_abi_dirent, d_name ) );

    uint32_t adjusted_size = offsetof(struct nacl_abi_dirent, d_name) + namelength +1 /* NUL termination */;
    adjusted_size = ROUND_UP( adjusted_size, 8 );
    if ( adjusted_size < buf_size ){
        dirent->d_reclen = ROUND_UP( adjusted_size, 8 );
        dirent->d_ino = d_ino;
        if ( d_off == 0x7fffffff )
            dirent->d_off = 0x7fffffff;
        else
            dirent->d_off = d_off+dirent->d_reclen;

        memcpy( dirent->d_name, d_name, namelength );
        ((char*)dirent->d_name)[namelength] = '\0';
        zrt_log( "dirent: name=%s, ino=%u, d_off=%u, d_reclen=%d, namel=%d",
                d_name, (unsigned int)d_ino, (unsigned int)d_off, dirent->d_reclen, namelength );
        return dirent->d_reclen;
    }
    else{
        zrt_log("no enough buffer, data_size=%d, buf_size=%d", adjusted_size, buf_size);
        return -1; /*no enough buffer size*/
    }
}


/*
 *@return readed bytes count*/
int readdir_to_buffer( int dir_handle, char *buf, int bufsize, struct ReadDirTemp *readdir_temp,
        struct UserManifest *manifest, struct manifest_loaded_directories_t *dirs){
    assert( readdir_temp ); /*should always exist*/
    zrt_log("temp handle=%d, dir_last_index=%d, channel_last_index=%d",
            readdir_temp->dir_data.handle, readdir_temp->dir_last_readed_index, readdir_temp->channel_last_readed_index);

    int retval = 0;
    int foo;
    /*if launched getdents for new directory*/
    if ( readdir_temp->dir_data.handle != dir_handle ){
        struct dir_data_t *d = match_handle_in_directory_list(dirs, dir_handle);
        assert(d); /*bad handle should be handed on upper level*/
        readdir_temp->dir_data = *d;
        readdir_temp->dir_last_readed_index = 0;
        readdir_temp->channel_last_readed_index = 0;
        zrt_log("new readdir call: handle=%d, path= %s", readdir_temp->dir_data.handle, readdir_temp->dir_data.path );
    }
    else if ( readdir_temp->dir_last_readed_index == -1 && readdir_temp->channel_last_readed_index == -1 ){
        /*come last of subsequent calls, now reset handle*/
        readdir_temp->dir_data.handle = -1;
        return 0;
    }

    /*if currently is in reading state of specified handle and also
     *dir/channel _last_related_index > 0 then start/continue readdir, else if < 0 then reading complete*/
    if ( readdir_temp->dir_data.handle == dir_handle && readdir_temp->dir_last_readed_index >= 0 ){
        /*add sub dirs*/
        do{
            zrt_log( "dir_index=%d", readdir_temp->dir_last_readed_index );
            readdir_temp->dir_last_readed_index =
                    get_sub_dir_index( dirs, readdir_temp->dir_data.path,
                            readdir_temp->dir_last_readed_index );
            zrt_log( "dir_index=%d", readdir_temp->dir_last_readed_index );

            if ( readdir_temp->dir_last_readed_index != -1 ){
                /*fetch name from full path*/
                const char *adding_name = name_from_path_get_path_len(
                        dirs->dir_array[readdir_temp->dir_last_readed_index].path, &foo );

                int w = put_dirent_into_buf(
                        buf+retval, bufsize-retval,
                        INODE_FROM_HANDLE(dirs->dir_array[readdir_temp->dir_last_readed_index].handle),
                        0, /*d_off*/
                        adding_name, strlen( adding_name ) );
                if ( w == -1 ) return retval;
                retval += w;
                readdir_temp->dir_last_readed_index++;
                zrt_log( "retval =%d", retval );
            }
        }while( readdir_temp->dir_last_readed_index >= 0 );
        readdir_temp->dir_last_readed_index =-1;
    }

    if( readdir_temp->dir_data.handle == dir_handle && readdir_temp->channel_last_readed_index >= 0 ){
        /*add directory files*/
        do{
            zrt_log( "channel_index=%d", readdir_temp->channel_last_readed_index );
            readdir_temp->channel_last_readed_index =
                    get_dir_content_channel_index(manifest->channels, manifest->channels_count,
                            readdir_temp->dir_data.path, readdir_temp->channel_last_readed_index);
            zrt_log( "channel_index=%d", readdir_temp->channel_last_readed_index );

            if ( readdir_temp->channel_last_readed_index != -1 ){
                /*fetch name from full path*/
                const char *adding_name = name_from_path_get_path_len(
                        ( const char*)manifest->channels[readdir_temp->channel_last_readed_index].name, &foo );

                int w = put_dirent_into_buf(
                        buf+retval, bufsize-retval,
                        INODE_FROM_HANDLE(readdir_temp->channel_last_readed_index),
                        0, /*d_off*/
                        adding_name, strlen( adding_name ) );
                if ( w == -1 ) return retval;
                retval += w;
                readdir_temp->channel_last_readed_index++;
                zrt_log( "retval =%d", retval );
            }
        }while( readdir_temp->channel_last_readed_index >= 0 );
        readdir_temp->channel_last_readed_index = -1;
    }

    return retval;
}

