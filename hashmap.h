#ifndef hashmap_h
#define hashmap_h

struct HashMap {
    void* buckets;
    int NBuckets;
    int N;
};


/**
 * @brief Dynamically allocate memory for a hashmap and all of its buckets
 * 
 * @return struct HashMap* 
 */
struct HashMap* HashMap_init();

/**
 * @brief Free all linked lists associated to a hash map,
 * and then finally the hash map itself
 * 
 * @param map 
 */
void HashMap_free(struct HashMap* map);


/**
 * @brief Put a key/value pair in a hash map, or update
 * the value associated to a key if it's already there
 * 
 * @param key Key
 * @param value Value
 */
void HashMap_put(struct HashMap* map, char* key, void* value);

/**
 * @brief Return the value associated to a key, or NULL
 * if the key does not exist in the map
 * 
 * @param map 
 * @param key 
 * @return void* 
 */
void* HashMap_get(struct HashMap* map, char* key);

/**
 * @brief Print out a string representation of the hashmap for debugging
 * 
 * @param map 
 */
void HashMap_print(struct HashMap* map);

#endif