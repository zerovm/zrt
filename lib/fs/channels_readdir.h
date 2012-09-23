/*
 * readdir.h
 *
 *  Created on: 03.08.2012
 *      Author: yaroslav
 */

#ifndef CHANNELS_READDIR_H_
#define CHANNELS_READDIR_H_

#define DIRECTORIES_MAX_NUMBER 255

//forwards
struct ZVMChannel;

struct dir_data_t {
    /*directory handle should be in range starting just after channels fd range;
     * can be used also to get inode by INODE_FROM_HANDLE*/
    int handle;
    int nlink;
    char* path;
    uint32_t open_mode; /*for currently opened dir contains mode flags, always O_RDONLY*/
};

struct manifest_loaded_directories_t{
    struct dir_data_t dir_array[DIRECTORIES_MAX_NUMBER];
    int dircount;
};

/*used by getdents syscall*/
struct ReadDirTemp{
    struct dir_data_t dir_data;
    int dir_last_readed_index;
    int channel_last_readed_index;
};

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
int get_sub_dir_index( struct manifest_loaded_directories_t *manifest_dirs, const char *dirpath, int index );

/* Search channel related to dirpath starting from index in manifest_dirs, if matched return matched channel index
 * To search mor than one channel user would call it with index parameter returned by previous function call;
 * @return channel index if found, -1 if not*/
int get_dir_content_channel_index(const struct ZVMChannel *channels, int channels_count, const char *dirpath, int index);

/* read directory contents into buffer for getdents syscall
 * @param dir_handle directory to get it's content
 * readdir_temp for first launch int fields should be initialized by -1;
 *@return readed bytes count*/
int readdir_to_buffer( int dir_handle, char *buf, int bufsize, struct ReadDirTemp *readdir_temp,
        const struct ZVMChannel *channels, int channels_count, struct manifest_loaded_directories_t *dirs);

#endif /* CHANNELS_READDIR_H_ */
