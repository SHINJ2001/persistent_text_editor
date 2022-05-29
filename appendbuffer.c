tyepdef struct abuf {
    char *b;
    int len;
} abuf;

void initAbuf(abuf* ab, int Len){
    ab -> b = (char*)malloc(sizeof(char)*len);
    ab -> len = Len;
}

void appendAB(abuf *ab, char* string, int Len){
    char *new = realloc(ab->b, ab->len + len);
    memcpy(&new[ab->len], string, len);
    ab->b = new;
    ab->len += len;
}

void freeAB(abuf *ab){
    free(ab->b);
}
