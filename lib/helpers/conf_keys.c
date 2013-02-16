
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include "zrtlog.h"
#include "conf_keys.h"

/*****************************************
 * keys of params related to single record
 *****************************************/


static int add_key_to_list(struct KeyList* list, const char* key){
    assert(list);
    int i;
    for (i=0; i < MAX_KEYS_COUNT; i++){
	if ( list->keys[i] == NULL ){
	    /*alloc memory and copy key to list*/
	    list->keys[i] = calloc( strlen(key)+1, 1 );
	    strcpy( list->keys[i], key );
	    ZRT_LOG(L_SHORT, "%s", key);
	    break;
	}
    }
    /*if folowing assert is raised then just increase defined max observers count*/
    assert(i<MAX_KEYS_COUNT);
    return i; /*get index of added key*/
}

static int key_find(const struct KeyList* list, const char* key, int keylen){
    assert(list);
    int i;
    for(i=0; i < MAX_KEYS_COUNT; i++ ){
	if ( list->keys[i] != NULL ){
	    /*compare key ignoring case*/
	    int etalonkeylen = strlen(list->keys[i]);
	    if ( !strncasecmp(key, list->keys[i], etalonkeylen) &&
		 keylen == etalonkeylen ){
		return i; /*get key index, specified key macthed*/
	    }
	}
    }
    return -1; /*wrong key*/
}

static int key_count(const struct KeyList* list){
    assert(list);
    int i;
    for(i=0; i < MAX_KEYS_COUNT; i++ ){
	if ( list->keys[i] == NULL ){
	    break;
	}
    }
    return i;
}

static void free_keylist(struct KeyList* list){
    assert(list);
    int i;
    for( i=0; i < MAX_KEYS_COUNT; i++ ){
	free(list->keys[i]), list->keys[i]=NULL;
    }
    free(list);
}

/*****
 allocation */

struct KeyList* alloc_key_list(){
    struct KeyList* self = calloc(1, sizeof(struct KeyList));
    self->add_key = add_key_to_list;
    self->find = key_find;
    self->count = key_count;
    self->free = free_keylist;
    return self;
}

