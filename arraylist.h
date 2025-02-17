#ifndef ARRAYLIST_H
#define ARRAYLIST_H

typedef struct ArrayListBuf {
    char* buff;
    int capacity;
    int N;
} ArrayListBuf;

void ArrayListBuf_init(struct ArrayListBuf* b);
void ArrayListBuf_free(struct ArrayListBuf* b);
void ArrayListBuf_doubleCapacity(struct ArrayListBuf* b);
void ArrayListBuf_push(struct ArrayListBuf* b, char* s, int len);

#endif
