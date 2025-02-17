#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"

#define START_BUCKETS 64


struct Node {
    struct Node* next;
    char* key;
    void* value;
};

struct Bucket {
    struct Node* head;
};

struct Bucket* Bucket_init() {
    struct Bucket* bucket = (struct Bucket*)malloc(sizeof(struct Bucket));
    bucket->head = NULL;
    return bucket;
}

void Bucket_free(struct Bucket* bucket) {
    // Step 1: Clean up nodes
    struct Node* node = bucket->head;
    while (node != NULL) {
        struct Node* nextNode = node->next;
        free(node);
        node = nextNode;
    }
    // Step 2: Free bucket
    free(bucket);
}

void Bucket_print(struct Bucket* bucket) {
    struct Node* node = bucket->head;
    printf("==>");
    while (node != NULL) {
        printf("(%s, %s) ==> ", node->key, (char*)node->value);
        node = node->next;
    }
    printf("\n");
}

void Bucket_addFirst(struct Bucket* bucket, char* key, void* value) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->key = key;
    newNode->value = value;
    newNode->next = bucket->head;
    bucket->head = newNode;
}

int Bucket_put(struct Bucket* bucket, char* key, void* value) {
    struct Node* node = bucket->head;
    int added = 1;
    while (node != NULL && added == 1) {
        if (strcmp(key, node->key) == 0) {
            // Key is already there, so just update value
            added = 0;
            node->value = value;
        }
        if (added == 1) {
            node = node->next;
        }
    }
    if (node == NULL) {
        Bucket_addFirst(bucket, key, value);
    }
    return added;
}

void* Bucket_get(struct Bucket* bucket, char* key) {
    void* ret = NULL;
    struct Node* node = bucket->head;
    int found = 0;
    while (node != NULL && found == 0) {
        if (strcmp(key, node->key) == 0) {
            // Key is already there, so just update value
            found = 1;
            ret = node->value;
        }
        node = node->next;
    }
    return ret;
}



/**
 * @brief Dynamically allocate memory for a hashmap and all of its buckets
 * 
 * @return struct HashMap* 
 */
struct HashMap* HashMap_init() {
    struct HashMap* map = (struct HashMap*)malloc(sizeof(struct HashMap));
    map->NBuckets = START_BUCKETS;
    map->N = 0;
    map->buckets = malloc(sizeof(struct Bucket*)*map->NBuckets);
    struct Bucket** buckets = (struct Bucket**)map->buckets;
    for (int i = 0; i < map->NBuckets; i++) {
        buckets[i] = Bucket_init();
    }
    return map;
}

/**
 * @brief Free all linked lists associated to a hash map,
 * and then finally the hash map itself
 * 
 * @param map 
 */
void HashMap_free(struct HashMap* map) {
    struct Bucket** buckets = (struct Bucket**)map->buckets;
    for (int i = 0; i < map->NBuckets; i++) {
        Bucket_free(buckets[i]);
    }
    free(buckets);
    free(map);
}

/**
 * @brief Return the hash code for a string
 * 
 * @param s String of which to compute hash code
 * @return long Hash code
 */
long charHash(char* s) {
    long hash = 0;
    while ((*s) != '\0') {
        hash = 31*hash + *s;
        s++;
    }
    return hash;
}

/**
 * @brief Put a key/value pair in a hash map, or update
 * the value associated to a key if it's already there
 * 
 * @param key Key
 * @param value Value
 */
void HashMap_put(struct HashMap* map, char* key, void* value) {
    long i = charHash(key) % map->NBuckets;
    struct Bucket** buckets = (struct Bucket**)map->buckets;
    map->N += Bucket_put(buckets[i], key, value);
}

/**
 * @brief Return the value associated to a key, or NULL
 * if the key does not exist in the map
 * 
 * @param map 
 * @param key 
 * @return void* 
 */
void* HashMap_get(struct HashMap* map, char* key) {
    long i = charHash(key) % map->NBuckets;
    struct Bucket** buckets = (struct Bucket**)map->buckets;
    return Bucket_get(buckets[i], key);
}

/**
 * @brief Print out a string representation of the hashmap for debugging
 * 
 * @param map 
 */
void HashMap_print(struct HashMap* map) {
    struct Bucket** buckets = (struct Bucket**)map->buckets;
    for (int i = 0; i < map->NBuckets; i++) {
        Bucket_print(buckets[i]); 
    }
}
