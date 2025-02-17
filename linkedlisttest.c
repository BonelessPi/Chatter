#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"

int main() {
    struct LinkedList* list = LinkedList_init();
    char* chris = "chris";
    char* celia = "celia";
    char* james = "james";
    char* theo = "theo";
    char* layla = "layla";
    char* meowgi = "meowgi";
    char* violet = "violet";
    LinkedList_addFirst(list, chris);
    LinkedList_addFirst(list, celia);
    LinkedList_addFirst(list, james);
    LinkedList_addFirst(list, theo);
    LinkedList_addFirst(list, layla);
    LinkedList_addFirst(list, meowgi);
    LinkedList_addFirst(list, violet);

    LinkedList_print(list);
    LinkedList_remove(list, meowgi);
    LinkedList_print(list);
    LinkedList_remove(list, violet);
    LinkedList_print(list);
    LinkedList_remove(list, chris);
    LinkedList_print(list);
    LinkedList_remove(list, layla);
    LinkedList_remove(list, james);
    LinkedList_remove(list, theo);
    LinkedList_remove(list, celia);
    LinkedList_print(list);


    LinkedList_free(list);
}
