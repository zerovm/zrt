/*
 * common_channels_conf.c
 *
 *  Created on: 16.07.2012
 *      Author: yaroslav
 *      Temporary, while can't be loaded from channels via zrt api
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdio.h> //fprintf
#include <unistd.h> //close
#include <fcntl.h> //open
#include <string.h>
#include <assert.h>
#include "defines.h"

#include "channels_conf.h"
#include "common_channels_conf.h"

#define TMP_PATH_MAX_LEN 100
char __temp_path[TMP_PATH_MAX_LEN+1];

struct UserChannel __config[] = {
		{ .srcnodeid=1, .dstnodeid=STDIN,  .fd=0, .mode=CHANNEL_MODE_READ, .path="r_map1_stdin" }, //map_node#1<-stdin
		{ .srcnodeid=1, .dstnodeid=5, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map1_red5" }, //map_node#1->reduce_node#1
		{ .srcnodeid=1, .dstnodeid=6, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map1_red6" }, //map_node#1->reduce_node#2
		{ .srcnodeid=1, .dstnodeid=7, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map1_red7" }, //map_node#1->reduce_node#3
		{ .srcnodeid=1, .dstnodeid=8, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map1_red8" }, //map_node#1->reduce_node#4
		{ .srcnodeid=1, .dstnodeid=9, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map1_red9" }, //map_node#1->reduce_node#5
		{ .srcnodeid=1, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map1_map2" }, //map_node#1->map_node#2
		{ .srcnodeid=1, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map1_map3" }, //map_node#1->map_node#3
		{ .srcnodeid=1, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map1_map4" }, //map_node#1->map_node#4
		{ .srcnodeid=1, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map1_map2" }, //map_node#1<-map_node#2
		{ .srcnodeid=1, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map1_map3" }, //map_node#1<-map_node#3
		{ .srcnodeid=1, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map1_map4" }, //map_node#1<-map_node#4
		{ .srcnodeid=2, .dstnodeid=STDIN,  .fd=0, .mode=CHANNEL_MODE_READ, .path="r_map2_stdin" }, //map_node#2<-stdin
		{ .srcnodeid=2, .dstnodeid=5, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map2_red5" }, //map_node#2->reduce_node#1
		{ .srcnodeid=2, .dstnodeid=6, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map2_red6" }, //map_node#2->reduce_node#2
		{ .srcnodeid=2, .dstnodeid=7, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map2_red7" }, //map_node#2->reduce_node#3
		{ .srcnodeid=2, .dstnodeid=8, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map2_red8" }, //map_node#2->reduce_node#4
		{ .srcnodeid=2, .dstnodeid=9, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map2_red9" }, //map_node#2->reduce_node#5
		{ .srcnodeid=2, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map2_map1" }, //map_node#2->map_node#1
		{ .srcnodeid=2, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map2_map3" }, //map_node#2->map_node#3
		{ .srcnodeid=2, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map2_map4" }, //map_node#2->map_node#4
		{ .srcnodeid=2, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map2_map1" }, //map_node#2<-map_node#1
		{ .srcnodeid=2, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map2_map3" }, //map_node#2<-map_node#3
		{ .srcnodeid=2, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map2_map4" }, //map_node#2<-map_node#4
		{ .srcnodeid=3, .dstnodeid=STDIN,  .fd=0, .mode=CHANNEL_MODE_READ, .path="r_map3_stdin" }, //map_node#3<-stdin
		{ .srcnodeid=3, .dstnodeid=5, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map3_red5" }, //map_node#3->reduce_node#1
		{ .srcnodeid=3, .dstnodeid=6, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map3_red6" }, //map_node#3->reduce_node#2
		{ .srcnodeid=3, .dstnodeid=7, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map3_red7" }, //map_node#3->reduce_node#3
		{ .srcnodeid=3, .dstnodeid=8, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map3_red8" }, //map_node#3->reduce_node#4
		{ .srcnodeid=3, .dstnodeid=9, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map3_red9" }, //map_node#3->reduce_node#5
		{ .srcnodeid=3, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map3_map1" }, //map_node#3->map_node#1
		{ .srcnodeid=3, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map3_map2" },  //map_node#3->map_node#2
		{ .srcnodeid=3, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map3_map4" },  //map_node#3->map_node#4
		{ .srcnodeid=3, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map3_map1" }, //map_node#3<-map_node#1
		{ .srcnodeid=3, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map3_map2" }, //map_node#3<-map_node#2
		{ .srcnodeid=3, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map3_map4" }, //map_node#3<-map_node#4
		{ .srcnodeid=4, .dstnodeid=STDIN,  .fd=0, .mode=CHANNEL_MODE_READ, .path="r_map4_stdin" }, //map_node#4<-stdin
		{ .srcnodeid=4, .dstnodeid=5, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map4_red5" }, //map_node#3->reduce_node#1
		{ .srcnodeid=4, .dstnodeid=6, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map4_red6" }, //map_node#3->reduce_node#2
		{ .srcnodeid=4, .dstnodeid=7, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map4_red7" }, //map_node#3->reduce_node#3
		{ .srcnodeid=4, .dstnodeid=8, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map4_red8" }, //map_node#3->reduce_node#4
		{ .srcnodeid=4, .dstnodeid=9, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map4_red9" }, //map_node#3->reduce_node#5
		{ .srcnodeid=4, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map4_map1" }, //map_node#3->map_node#1
		{ .srcnodeid=4, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map4_map2" },  //map_node#3->map_node#2
		{ .srcnodeid=4, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_WRITE, .path="w_map4_map3" },  //map_node#3->map_node#3
		{ .srcnodeid=4, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map4_map1" }, //map_node#3<-map_node#1
		{ .srcnodeid=4, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map4_map2" }, //map_node#3<-map_node#2
		{ .srcnodeid=4, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_map4_map3" }, //map_node#3<-map_node#3
		{ .srcnodeid=5, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red5_map1" }, //reduce_node#1<-map_node#1
		{ .srcnodeid=5, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red5_map2" }, //reduce_node#1<-map_node#2
		{ .srcnodeid=5, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red5_map3" }, //reduce_node#1<-map_node#3
		{ .srcnodeid=5, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red5_map4" }, //reduce_node#1<-map_node#4
		{ .srcnodeid=6, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red6_map1" }, //reduce_node#2<-map_node#1
		{ .srcnodeid=6, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red6_map2" }, //reduce_node#2<-map_node#2
		{ .srcnodeid=6, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red6_map3" }, //reduce_node#2<-map_node#3
		{ .srcnodeid=6, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red6_map4" }, //reduce_node#2<-map_node#4
		{ .srcnodeid=7, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red7_map1" }, //reduce_node#3<-map_node#1
		{ .srcnodeid=7, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red7_map2" }, //reduce_node#3<-map_node#2
		{ .srcnodeid=7, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red7_map3" }, //reduce_node#3<-map_node#3
		{ .srcnodeid=7, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red7_map4" }, //reduce_node#3<-map_node#4
		{ .srcnodeid=8, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red8_map1" }, //reduce_node#4<-map_node#1
		{ .srcnodeid=8, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red8_map2" }, //reduce_node#4<-map_node#2
		{ .srcnodeid=8, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red8_map3" }, //reduce_node#4<-map_node#3
		{ .srcnodeid=8, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red8_map4" }, //reduce_node#5<-map_node#4
		{ .srcnodeid=9, .dstnodeid=1, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red9_map1" }, //reduce_node#5<-map_node#1
		{ .srcnodeid=9, .dstnodeid=2, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red9_map2" }, //reduce_node#5<-map_node#2
		{ .srcnodeid=9, .dstnodeid=3, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red9_map3" }, //reduce_node#5<-map_node#3
		{ .srcnodeid=9, .dstnodeid=4, .fd=0, .mode=CHANNEL_MODE_READ , .path="r_red9_map4" } //reduce_node#5<-map_node#4

};


//	uint16_t srcnodeid;
//	uint16_t dstnodeid;
//	int16_t fd;
//	int8_t mode;
//	char *path;sizeof

/*Fill channels list the same for all nodes. Need to be called for every node*/
void FillUserChannelsList( struct ChannelsConfInterface *ch_in ){
	int conf_count = sizeof(__config)/sizeof(struct UserChannel);
	char *path = __temp_path;
#ifdef USER_SIDE
	int dirnamelen = strlen(ZEROVM_DEBUG_CHANNELS_PATH);
#else
	int dirnamelen = strlen(GCC_DEBUG_CHANNELS_PATH);
#endif
	struct UserChannel *current = NULL;
	for ( int i=0; i < conf_count; i++ ){
		current = &__config[i];
#ifdef USER_SIDE
		if ( current->dstnodeid == STDIN ) continue; //already actual value = 0
		assert( TMP_PATH_MAX_LEN > strlen(current->path) + dirnamelen  );
		memset( path, '\0', TMP_PATH_MAX_LEN );
		memcpy( path, ZEROVM_DEBUG_CHANNELS_PATH, dirnamelen );
		memcpy( path+dirnamelen, current->path, strlen(current->path) );
#else
		assert( TMP_PATH_MAX_LEN > strlen(current->path) + dirnamelen  );
		memset( path, '\0', TMP_PATH_MAX_LEN );
		memcpy( path, GCC_DEBUG_CHANNELS_PATH, dirnamelen );
		memcpy( path+dirnamelen, current->path, strlen(current->path) );
#endif
		if ( current->srcnodeid == ch_in->ownnodeid ){
			if ( current->mode == CHANNEL_MODE_WRITE ){
				current->fd = open( path, O_WRONLY | O_CREAT );
				WRITE_FMT_LOG("fd=%d open write file nodeid=%d, path=%s\n", current->fd,current->dstnodeid, path );
				assert( current->fd >= 0 );
			}
			else if ( current->mode == CHANNEL_MODE_READ ){
				current->fd = open( path, O_RDONLY );
				WRITE_FMT_LOG("fd=%d open file read nodeid=%d, path=%s\n", current->fd,current->dstnodeid, path );
			}
			else{
				assert(0);
			}
		}
	}

	for ( int i=0; i < conf_count; i++ ){
		ch_in->ChannelConfAdd( ch_in, &__config[i] );
	}
}


void CloseChannels(){
#ifndef USER_SIDE
	int conf_count = sizeof(__config)/sizeof(struct UserChannel);
	struct UserChannel *current = NULL;
	for ( int i=0; i < conf_count; i++ ){
		current = &__config[i];
		close( current->fd );
	}
#endif
}

