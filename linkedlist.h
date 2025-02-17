#ifndef LINKEDLIST_H
#define LINKEDLIST_H

struct LinkedNode {
    struct LinkedNode* next;
    void* data;
};

struct LinkedList {
    struct LinkedNode* head;
};

struct LinkedList* LinkedList_init();
void LinkedList_free(struct LinkedList* list);
void LinkedList_addFirst(struct LinkedList* list, void* data);
void* LinkedList_removeFirst(struct LinkedList* list);
/**
 * @brief Remove the first occurrence of an item if it's in the list
 * 
 * @param list List to search
 * @param data Data to remove
 * @return void* Data, or NULL if nothing found
 */
void* LinkedList_remove(struct LinkedList* list, void* data);
void LinkedList_print(struct LinkedList* list);

#endif