#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"


struct LinkedList* LinkedList_init() {
    struct LinkedList* list = (struct LinkedList*)malloc(sizeof(struct LinkedList));
    list->head = NULL;
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
    free(list);
}

void LinkedList_addFirst(struct LinkedList* list, void* data) {
    struct LinkedNode* newHead = (struct LinkedNode*)malloc(sizeof(struct LinkedNode));
    newHead->next = list->head;
    newHead->data = data;
    list->head = newHead;
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
