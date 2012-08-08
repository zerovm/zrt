/*
 * common_channels_conf.c
 *
 *  Created on: 16.07.2012
 *      Author: yaroslav
 *      map / reduce common configuration functions
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdio.h> //fprintf
#include <string.h>
#include <stdlib.h> //atoi
#include <unistd.h> //close
#include <fcntl.h> //open
#include <dirent.h>
#include <assert.h>

#include "defines.h"
#include "common_channels_conf.h"
#include "channels_conf.h"

char s_temp_path[MAX_PATH_LENGTH];

int ExtractNodeNameId( const char *nameall, int *nodenamelen ){
    int len = strlen(nameall);
    /*search rightmost delimemeter*/
    char *s = strrchr(nameall, '-');
    if ( s ){
        *nodenamelen = (int)(s-nameall);
        if ( *nodenamelen+1 < len ) return atoi(s+1); /*convert id*/
        else                        return -1;        /*id not supplied*/

    }
    else{
        *nodenamelen = strlen(nameall);
        return -1; /*no delimeter, no integer id*/
    }
}


const char *listdir(DIR* opened_dir) {
    struct dirent *entry;
    assert(opened_dir);

    while ( (entry = readdir(opened_dir)) ){
        return entry->d_name;
    }
    return NULL;
}


int AddAllChannelsRelatedToNodeTypeFromDir( struct ChannelsConfigInterface *chan_if,
        const char *dirpath, int channel_mode, int nodetype, const char* nodename_text ){
    int res=0;
    int extracted_len;
    int extracted_nodeid=0;
    const char *readdir_get_name = NULL;
    int fd=-1;
    /***********************************************************************
     open in channel, readed from /dev/in dir */
    DIR *dp = opendir( dirpath );
    while ( (readdir_get_name=listdir(dp)) && !res ){
        /*fetch node type name and node identifier*/
        extracted_nodeid = ExtractNodeNameId( readdir_get_name, &extracted_len );
        /*if nodenames are equal*/
        if ( extracted_len == strlen(nodename_text) && !strncmp(nodename_text, readdir_get_name, extracted_len ) )
        {
            snprintf( s_temp_path, MAX_PATH_LENGTH, "%s/%s", dirpath, readdir_get_name );
            if ( channel_mode == EChannelModeWrite ){
                fd = open( s_temp_path, O_WRONLY); /*open channel fetched from manifest configuration*/
            }
            else{
                fd = open( s_temp_path, O_RDONLY); /*open channel fetched from manifest configuration*/
            }
            if( chan_if->AddChannel( chan_if, nodetype, extracted_nodeid, fd, channel_mode ) == NULL ){
                WRITE_FMT_LOG("failed add channel to config: nodetype=%d, extrctednodeid=%d, fd=%d, mode=%d, path=%s\n",
                        nodetype, extracted_nodeid, fd, channel_mode, s_temp_path );
                res=-1;
                break;
            }else{
                WRITE_FMT_LOG("Add channel to config: nodetype=%d, extrctednodeid=%d, fd=%d, mode=%d, path=%s\n",
                        nodetype, extracted_nodeid, fd, channel_mode, s_temp_path );
            }
        }
    }

    closedir(dp);
    return res;
}


