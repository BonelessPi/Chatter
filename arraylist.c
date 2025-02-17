#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "arraylist.h"

#define INITIAL_SIZE 1024

void ArrayListBuf_init(struct ArrayListBuf* b) {
    b->buff = (char*)malloc(INITIAL_SIZE);
    b->capacity = INITIAL_SIZE;
    b->N = 0;
}

void ArrayListBuf_free(struct ArrayListBuf* b) {
    free(b->buff);
}

void ArrayListBuf_doubleCapacity(struct ArrayListBuf* b) {
    char* newBuff = (char*)malloc(b->capacity*2);
    memcpy(newBuff, b->buff, b->capacity);
    free(b->buff);
    b->buff = newBuff;
    b->capacity *= 2;
}

void ArrayListBuf_push(struct ArrayListBuf* b, char* s, int len) {
    while (b->N + len > b->capacity) {
        ArrayListBuf_doubleCapacity(b);
    }
    memcpy(b->buff+b->N, s, len);
    b->N += len;
}
