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

#include "zrtlog.h"
#include "helpers/dyn_array.h"
#include "channels_conf_reader.h"
#include "channels_conf.h"

#define MAX_PATH_LENGTH 255
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
    ZRT_LOG(L_SHORT, "Add directory=%s content items matched by pattern=%s", 
	    dirpath, nodename_text);
    DIR *dp = opendir( dirpath );
    while ( (readdir_get_name=listdir(dp)) && !res ){
        /*fetch node type name and node identifier*/
        extracted_nodeid = ExtractNodeNameId( readdir_get_name, &extracted_len );
        /*if iterated directory item matched with nodename pattern
	 *then extract handle of that directory item by opening of it's file
	 *and add it to channels config*/
	ZRT_LOG(L_EXTRA, "directory item=%s, len=%d", readdir_get_name, extracted_len );
        if ( extracted_len == strlen(nodename_text) 
	     && 
	     !strncmp(nodename_text, readdir_get_name, extracted_len ) )
        {
            snprintf( s_temp_path, MAX_PATH_LENGTH, "%s/%s", dirpath, readdir_get_name );
            if ( channel_mode == EChannelModeWrite )
                fd = open( s_temp_path, O_WRONLY); /*open channel fetched from manifest configuration*/
            else
                fd = open( s_temp_path, O_RDONLY); /*open channel fetched from manifest configuration*/
	    ZRT_LOG(L_SHORT, "matched item=%s by pattern=%s", readdir_get_name, nodename_text);
            assert(fd>=0);
            if ( extracted_nodeid == -1 ) extracted_nodeid = 1; /*for single node without id*/
            if( chan_if->AddChannel( chan_if, nodetype, extracted_nodeid, fd, channel_mode ) 
		== NULL ){
                res=-1;
                break;
            }
        }
    }

    closedir(dp);
    ZRT_LOG(L_SHORT, "channels added to config, res=%d by pattern=%s", res, nodename_text);
    return res;
}

void CloseChannels( struct ChannelsConfigInterface *chan_if ){
    for ( int i=0; i < chan_if->channels->num_entries;i++ ){
        struct UserChannel *channel = (struct UserChannel *)chan_if->channels->ptr_array[i];
        if ( channel ){
            close(channel->fd);  /*close all opened file descriptors*/
        }
    }
    chan_if->Free(chan_if); /*freee memories used by channels configuration*/
}


