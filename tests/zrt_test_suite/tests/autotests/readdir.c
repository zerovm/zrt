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


#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>

#include "zrtlog.h"
#include "zrtlogbase.h"
#include "path_utils.h"
#include "macro_tests.h"

#define DIRENTRY_NAME_MAX_LEN 64
typedef enum {
    EPipe=DT_FIFO, //1
    ECharDev=DT_CHR, //2
    EDir=DT_DIR,  //4
    EBlockDev=DT_BLK, //6
    EFile=DT_REG //8
} entrytype_t;
struct direntry_t{
    char name[DIRENTRY_NAME_MAX_LEN];
    entrytype_t type;
};

void readdir_test_engine(const char *path, const struct direntry_t expected[], int count );
void test_issue_48_dev();
void test_root();
void test_random_folder();

int main(){
    test_issue_48_dev();
    test_root();
    test_random_folder();
    return 0;
}

/*@return 0 if matched, -1 if not*/
static int match_direntry(struct dirent* entry, const struct direntry_t expected[], int count){
    int i;
    int ret;
    const struct direntry_t *current_test_data;
    LOG_STDERR(ELogPath, entry->d_name, "match_direntry name check");

    /*search entry in list of expected entries*/
    for ( i=0; i < count; i++ ){
	current_test_data = &expected[i];
	if ( !strcmp(entry->d_name, current_test_data->name ) ){
	    LOG_STDERR(ELogPath, entry->d_name, "name matched");
	    fprintf(stderr, "entry type=%d, expected type=%d\n", 
		    entry->d_type, current_test_data->type );
	    TEST_OPERATION_RESULT( entry->d_type==current_test_data->type, &ret, ret==1 );	    
	}
    }
    return 0;
}

void readdir_test_engine(const char *path, const struct direntry_t expected[], int count ){
    int ret;
    int count_of_matched=0;
    struct dirent *entry;
    LOG_STDERR(ELogPath, path, "test");
    DIR *dp = opendir(path);
    TEST_OPERATION_RESULT( dp!=NULL, &ret, ret==1 );
    
    while ( (entry = readdir(dp))){
	TEST_OPERATION_RESULT( match_direntry(entry, expected, count), &ret, ret==0 );
	++count_of_matched;
    }
    fprintf(stderr, "entries count=%d, expected count=%d\n", 
	    count_of_matched, count );
    TEST_OPERATION_RESULT( count==count_of_matched, &ret, ret==1 );

    closedir(dp);
}

void test_issue_48_dev(){
    const struct direntry_t slash_dev_expected[] = { 
	{"stdin", ECharDev}, //mapping enabled
	{"stdout", EPipe},
	{"debug", EPipe},
	{"trace", EPipe},
	{"alloc_report", EPipe},
	{"nvram", EBlockDev},
	{"readonly", EPipe},
	{"writeonly", EPipe},
	{"read-write", EBlockDev},
	{"mount", EDir},
	{"mount1", EPipe},
	{"mount2", EFile},
	{"null", ECharDev}, //mapping enabled
	{"full", EBlockDev},
	{"zero", EBlockDev},
	{"random", EBlockDev},
	{"urandom", EBlockDev}
    };
    readdir_test_engine("/dev", slash_dev_expected, sizeof(slash_dev_expected)/sizeof(struct direntry_t) );

    const struct direntry_t slash_dev_mount_expected[] = { 
	{"import.tar", EBlockDev},
	{"gcov.gcda.tar", EBlockDev}
    };
    readdir_test_engine("/dev/mount", slash_dev_expected, sizeof(slash_dev_mount_expected)/sizeof(struct direntry_t) );
}

void test_root(){
    /*In order to pass tests when running tests for coverage then it
      is need to know about path of source code which is a part of
      debugging info and the same path will be used when creating gcda
      files in zerovm session, usually always suppath starting from
      /home, but in case if /home folder doesn't exist then skip test
      which examines root contents */
    int cursor;
    int reslen;
    char *home = getenv("CURDIR");
    INIT_TEMP_CURSOR(&cursor);
    path_component_forward(&cursor, home, &reslen); //skip "/" root
    path_component_forward(&cursor, home, &reslen); //get home catalog 

    if ( !strncmp(home, "/home", reslen ) ){
	const struct direntry_t root_expected[] = { 
	    {"home", EDir}, //to be compatible with gcov (gcov creates /home dir)
	    {"dev", EDir},
	    {"tmp", EDir},
	};
	/*in case if tests running in non coverage mode, then folder
	  will not create automatically, create it here*/
	mkdir("/home", 0666); 
	readdir_test_engine("/", root_expected, sizeof(root_expected)/sizeof(struct direntry_t) );
    }
}

static void test_readdir_with_many_components(const char *parent_dir, 
					      int create_folders_count, 
					      int create_files_count ){
    struct direntry_t *expected 
	= malloc(sizeof(struct direntry_t)*(create_folders_count+create_files_count));
    char *tmpname = malloc(PATH_MAX);
    char intstr[20];
    int i;
    /*create folders and fill testing data structure*/
    for (i=0; i < create_folders_count; i++){
	snprintf(intstr, sizeof(intstr), "%d", i);
	snprintf(tmpname, PATH_MAX, "%s/%s", parent_dir, intstr);
	strcpy( expected[i].name, intstr );
	expected[i].type = EDir;
	mkdir(tmpname, 0666);
    }

    /*create files and fill testing data structure*/
    for (i=create_folders_count; i < create_files_count+create_folders_count; i++){
	snprintf(intstr, sizeof(intstr), "%d", i);
	snprintf(tmpname, PATH_MAX, "%s/%s", parent_dir, intstr);
	strcpy( expected[i].name, intstr );
	expected[i].type = EFile;
	CREATE_FILE(tmpname, intstr, strlen(intstr));
    }

    readdir_test_engine(parent_dir, expected, create_folders_count+create_files_count );
    free(tmpname);
}

void test_random_folder(){
    int ret;
    char tmpdirname[PATH_MAX] ="/tmp/1234-XXXXXX";
    TEST_OPERATION_RESULT((int)mkdtemp(tmpdirname), (int*)&ret, ret!=0 );
    
    test_readdir_with_many_components(tmpdirname, 100, 100);
}
