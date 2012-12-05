/*
 * readdir.h
 *
 *  Created on: 03.08.2012
 *      Author: yaroslav
 */

#ifndef CHANNELS_READDIR_H_
#define CHANNELS_READDIR_H_

#include <stddef.h> //size_t 

#define DIRECTORIES_MAX_NUMBER 255

//forwards
struct ZVMChannel;

struct dir_data_t {
    /*directory handle should be in range starting just after channels fd range;
     * can be used also to get inode by INODE_FROM_HANDLE*/
    int handle;
    int nlink;
    char* path;
    uint32_t flags; /*for currently opened dir contains mode flags, always O_RDONLY*/
};

struct manifest_loaded_directories_t{
    struct dir_data_t dir_array[DIRECTORIES_MAX_NUMBER];
    int dircount;
};

/*Get shortname from full name*/
const char* name_from_path_get_path_len(const char *fullpath, int *pathlen);

/*search dir in dir list*/
struct dir_data_t *
match_dir_in_directory_list(struct manifest_loaded_directories_t *manifest_dirs, const char *dirpath, int len);

/*search dir handle in dir list*/
struct dir_data_t *
match_handle_in_directory_list(struct manifest_loaded_directories_t *manifest_dirs, int handle);


/*reading channels list, fetch directories from channel path and add to manifest_dirs,
 * manifest_dirs should point to valid pointer*/
void process_channels_create_dir_list( const struct ZVMChannel *channels, int channels_count,
        struct manifest_loaded_directories_t *manifest_dirs );

/* Search subdir starting from index in manifest_dirs, if matched return matched subdir index
 * To search many subdirs user would call it with index parameter returned by previous function call;
 * @return subdir index if found, -1 if not*/
int get_sub_dir_index( struct manifest_loaded_directories_t *manifest_dirs, 
		       const char *dirpath, 
		       int index );

/*low level function, just fill derent structure by data and adjust structure size */
size_t put_dirent_into_buf( char *buf, int buf_size, unsigned long d_ino, unsigned long d_off,
			    const char *d_name, int namelength );

#endif /* CHANNELS_READDIR_H_ */
