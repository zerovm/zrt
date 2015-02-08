/*
 *
 * Copyright (c) 2015, Rackspace, Inc.
 * Author: Yaroslav Litvinov, yaroslav.litvinov@rackspace.com
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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "zrtlog.h"
#include "fuse.h"

enum { 
    EConditionWaitingInitialization=0,
    EConditionInitialized=1,
    EConditionWaitingFinalization=2 
};

struct async_lock_data{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int             cond_value;
    const char     *name;
};

struct mount_args{
    char   mount_point[100];
    int    mount_argc;
    char **mount_argv;
    int  (*mount_main)(int, char**);
};

/*thread data stored in static variable, so only one fuse thread (fuse
  fs) is supported*/
struct async_lock_data s_lock_data = {PTHREAD_MUTEX_INITIALIZER,
                                      PTHREAD_COND_INITIALIZER,
                                      EConditionWaitingInitialization,NULL};

void wait_cond_value(struct async_lock_data * lock_data, int expected_cond_value)
{
    pthread_mutex_lock( &lock_data->mutex );
    ZRT_LOG(L_INFO, "condition='%s' current value=%d, wait value=%d", 
	    lock_data->name, lock_data->cond_value, expected_cond_value ); 
    while( lock_data->cond_value != expected_cond_value )
	pthread_cond_wait( &lock_data->cond, &lock_data->mutex );
    pthread_cond_signal( &lock_data->cond );      
    ZRT_LOG(L_INFO, "condition='%s' new value=%d", 
	    lock_data->name, lock_data->cond_value ); 
    pthread_mutex_unlock( &lock_data->mutex );
}

void update_cond_value(struct async_lock_data * lock_data, int new_cond_value)
{
    pthread_mutex_lock( &lock_data->mutex );
    ZRT_LOG(L_INFO, "update condition='%s' current value=%d, new value=%d", 
	    lock_data->name, lock_data->cond_value, new_cond_value ); 
    lock_data->cond_value = new_cond_value;
    pthread_cond_signal( &lock_data->cond );      
    ZRT_LOG(L_INFO, "update condition='%s' current value=%d, new value=%d", 
	    lock_data->name, lock_data->cond_value, new_cond_value ); 
    pthread_mutex_unlock( &lock_data->mutex );
}

fs_main
fusefs_entrypoint_get_args_by_fsname(const char *fsname, 
				     int *argc, char ***argv)
{
#ifdef FUSE_SQLARFS_NAME
    if ( !strncasecmp(fsname, FUSE_SQLARFS_NAME, sizeof(FUSE_SQLARFS_NAME)) ){
	FUSE_SQLARFS_ARGS_FILL(argc, argv);
	return sqlarfs_main;
    }
#endif
#ifdef FUSE_ARCHIVEMOUNT_NAME
    if ( !strncasecmp(fsname, FUSE_ARCHIVEMOUNT_NAME, sizeof(FUSE_ARCHIVEMOUNT_NAME)) ){
	FUSE_ARCHIVEMOUNT_ARGS_FILL(argc, argv);
	return archivemount_main;
    }
#endif
    return NULL;
}

/*thread asynchronous func, called from mount_fuse*/
static void *mount_fusefs_async(void *arg)
{
    struct mount_args *args = (struct mount_args *)arg;
    /*run fusefs's entry point,
     Here we will wait until app termination*/
    int ret = args->mount_main(args->mount_argc, args->mount_argv);
    if (ret){
	ZRT_LOG(L_ERROR, "fuse mount error=%d", ret ); 

    }
    /*Time to destroy? unmount, cleanup filesystem*/
    free(args);
    return NULL;
}

int fuse_main(int argc, char **argv, struct fuse_operations *op, void *user_data){
    /*perform filesystem mount here*/
    
    /*when it finishes, update cond to continue zrt running*/
    update_cond_value(&s_lock_data, EConditionInitialized);

    /*wait here until exit*/
    wait_cond_value( &s_lock_data, EConditionWaitingFinalization);
    return 0;
}


/*this a wraper around filesystem's main() entry point*/
int mount_fusefs(const char *mount_point,
		 int (*fs_main)(int, char**), int fs_argc, char **fs_argv){
    /*prepare parameters for thread function*/
    /*just hardcode it for first time*/
    struct mount_args *args = calloc(1, sizeof(struct mount_args));
    strncpy(args->mount_point, mount_point, sizeof(args->mount_point) );
    args->mount_argc = fs_argc;
    args->mount_argv = fs_argv;
    args->mount_main = fs_main;

    pthread_t thread_create_fuse;
    if (pthread_create(&thread_create_fuse, NULL, mount_fusefs_async, args) != 0){
	return -1;
    }
    /*Wait while filesystem initialization is done, it is signalled by
      s_lock_data.cond. It is expected fuse_main to be called
      inside mount_main, and then it releases condition below*/
    wait_cond_value( &s_lock_data, EConditionInitialized);
    ZRT_LOG(L_INFO, "%s", "fuse fs initializer finished" ); 
    return 0;
}

void mount_exit(){
    /*if exist initialized fs that need to be stopped*/
    if (s_lock_data.cond_value > EConditionWaitingInitialization){
	/*release waiting fuse_main, continue exiting*/
	update_cond_value(&s_lock_data, EConditionWaitingFinalization);
    }
}
