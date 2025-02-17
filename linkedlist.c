#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"

#define DEBUG 1
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

struct LinkedList* LinkedList_init() {
    struct LinkedList* list = (struct LinkedList*)malloc(sizeof(struct LinkedList));
    list->head = NULL;
    debug_print("linked list init: %p\n",(void*)list);
    return list;
}

void LinkedList_free(struct LinkedList* list) {
    // Step 1: Clean up nodes
    struct LinkedNode* node = list->head;
    while (node != NULL) {
        struct LinkedNode* nextNode = node->next;
        free(node);
        node = nextNode;
    }
    // Step 2: Free list
    debug_print("linked list free: %p\n",(void*)list);
    free(list);
}

void LinkedList_addFirst(struct LinkedList* list, void* data) {
    struct LinkedNode* newHead = (struct LinkedNode*)malloc(sizeof(struct LinkedNode));
    newHead->next = list->head;
    newHead->data = data;
    list->head = newHead;
    debug_print("linked list add first: %p\n",(void*)list);
}

void* LinkedList_removeFirst(struct LinkedList* list) {
    void* ret = NULL;
    if (list->head != NULL) {
        ret = list->head->data;
        void* tmp = list->head;
        list->head = list->head->next;
        free(tmp);
    }
    return ret;
    debug_print("linked list remove first: %p\n",(void*)list);
}

void* LinkedList_remove(struct LinkedList* list, void* data) {
    void* ret = NULL;
    if (list->head != NULL) {
        struct LinkedNode* node = list->head;
        if (node->data == data) {
            ret = LinkedList_removeFirst(list);
        }
        else {
            while (node->next != NULL) {
                if (node->next->data == data) {
                    struct LinkedNode* temp = node->next;
                    node->next = node->next->next;
                    ret = temp->data;
                    free(temp);
                    break;
                }
                else {
                    node = node->next;
                }
            }
        }
    }
    debug_print("linked list remove: %p\n",(void*)list);
    return ret;
}

void LinkedList_print(struct LinkedList* list) {
    struct LinkedNode* node = list->head;
    while (node != NULL) {
        printf("%s ==> ", (char*)node->data);
        node = node->next;
    }
    printf("\n");
}
