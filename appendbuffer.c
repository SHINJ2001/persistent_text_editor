#include "headers.h"
void initAbuf(abuf* ab){
    ab -> b = NULL;
    ab->len = 0;
}

void appendAB(abuf *ab, char* string, int Len){
    char *new = realloc(ab->b, ab->len + Len);
    memcpy(&new[ab->len], string, Len);
    ab->b = new;
    ab->len += Len;
}

void freeAB(abuf *ab){
    free(ab->b);
}
