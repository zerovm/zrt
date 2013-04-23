#ifndef CONF_KEYS_H
#define CONF_KEYS_H

#include "nvram.h"

/*Expected keys for single record in config file. */
struct KeyList{
    //functions
    int (*add_key)(struct KeyList* list, const char* key);
    /*@return index of key in array if specified key is found, -1 if not*/
    int (*find)(const struct KeyList* list, const char* key, int keylen);
    //preallocated space for keys
    char keys[NVRAM_MAX_KEYS_COUNT_IN_RECORD][NVRAM_MAX_KEY_LENGTH];
    int  count;
};

/*assign functions pointers for keys struct and return it
 *@param keys to construct*/
void keys_construct(struct KeyList* keys);

#endif //CONF_KEYS_H
