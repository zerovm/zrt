#ifndef CONF_KEYS_H
#define CONF_KEYS_H

#define MAX_KEYS_COUNT 4

/*list of waiting keys in parsing data.
 *parsing parameter is pair key=value.*/
struct KeyList{
    char*  keys[MAX_KEYS_COUNT];
    //functions
    int (*add_key)(struct KeyList* list, const char* key);
    /*@return index of key in array if specified key is found, -1 if not*/
    int (*find)(const struct KeyList* list, const char* key, int keylen);
    /*get actual keys count*/
    int (*count)(const struct KeyList* list);
    void (*free)(struct KeyList* list);
};

struct KeyList* alloc_key_list();

#endif //CONF_KEYS_H
