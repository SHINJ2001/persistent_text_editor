#include "headers.h"

typedef struct abuf {
    char *b;
    int len;
} abuf;

void initAbuf(abuf* ab);
void appendAB(abuf*, char*, int);
void freeAB(abuf*);
