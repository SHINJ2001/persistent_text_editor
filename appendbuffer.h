#include "headers.h"

typedef struct abuf {
    char *b;
    int len;
} abuf;

void initAbuf(abuf* ab, int Len);
void appendeAB(abuf*, char*, int);
void freeAB(abuf*);
