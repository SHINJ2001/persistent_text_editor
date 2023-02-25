#include "headers.h"

editor_config E;
int quit = 2;

void init_editor(){
    E.cx = 0; //Cursor positions
    E.cy = 0;

    E.numrows = 0; //Number of editor rows
    E.row = NULL;
    E.first = 1;
    E.row_offset = 0;
    E.col_offset = 0;
    E.version = 0;
    E.dirty = 0;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;
    E.rx = 0;
    E.filename[0] = '\0';
    E.count = 1;

    if(getWindowSize(&E.screenrow, &E.screencol) == -1) die("getWindowSize");// Writing window size to  struct
    E.screenrow -= 2;

}

void enableRawMode() {

    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | IXON | ICRNL | INPCK | ISTRIP ); //Prevents ctrl s and ctrl q from suspending program fkfk
    raw.c_oflag &= ~OPOST;
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(ECHO | 
            ICANON |  //Allows us to read input byte by byte instead of line by line and turns off echo
            ISIG | 
            IEXTEN); //Allows deactivating ctrl c and ctrl z commands

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetatttr");
}

void die(const char *msg){
    editorRefreshScreen();
    perror(msg);
    exit(1);
}

int editorReadKey(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1))!= 1)
        if((nread == -1)&&(errno != EAGAIN)) die("read");

    if(c == '\x1b'){
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        
        if(seq[0] == '['){
            if(seq[1] >= '0' &&seq[1] <= '9'){
                if(read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if(seq[2] == '~'){
                    switch(seq[2]){
                        case '3': return DEL_KEY;
                    }
                }
            } 
            else {
                switch(seq[1]){
                    case 'A': return ARROW_UP; //arrow up
                    case 'B': return ARROW_DOWN; //arrow down
                    case 'C': return ARROW_RIGHT; //arrow right
                    case 'D': return ARROW_LEFT; //arrow left
                }
            }

            return '\x1b';
        }
    }
    else{
        return c; //Return char if not a control sequence
    }

}

/*Recognize what key was pressed*/
void editorKeyPress(){
    int c = editorReadKey();
    int row, col;

    switch(c) {
        case '\r':
            editorInsertNewline();
            break;

        case (CTRL_key('q')):
            if(E.dirty && quit){
                editorSetStatusMessage("Unsaved changes in file. Press CTRL+q %d more times to quit anyway", quit);
                quit--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case BACKSPACE:
        case DEL_KEY:
            if(c == DEL_KEY) moveCursor(ARROW_RIGHT);
            editorDelChar();
            break;
        
        case CTRL_key('d'):
            GetCursorPos(&row, &col);
            delRow(row);
            break;

        case CTRL_key('s'):
            editorSave();
            break;

        case CTRL_key('o'):
            editorOpen(NULL, 0);
            break;
        
        case '\x1b':
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
            moveCursor(c);
            break;
        default:
            insertChar(c);
            break;
    }
}

/*Refreshes screen constantly to reflect the changes we make on the screen*/
void editorRefreshScreen(){
    editorScroll();

    abuf ab;
    initAbuf(&ab);

    appendAB(&ab, "\x1b[?25l", 6);
    appendAB(&ab, "\x1b[H", 3);
    
    drawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "\x1b[%d;%dH",(E.cy - E.row_offset) + 1, (E.rx - E.col_offset + 1));
    appendAB(&ab, buffer, strlen(buffer));

    appendAB(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    freeAB(&ab);
}

/*Returns where the cursor is currently pointing at*/
int GetCursorPos(int *rows, int* cols){

    char buf[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    printf("\r\n");

    while(i < sizeof(buf) - 1){
        if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
        i++;
    }
    buf[i] = '\0';

    if(buf[0] != '\x1b' || buf[1]  != '[') return -1;
    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

void moveCursor(int key){
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch(key){
        case ARROW_UP:
            if(E.cy != 0)
                E.cy--;
            break;
        case ARROW_DOWN:
            if(E.cy < E.numrows)
                E.cy++;
            break;
        case ARROW_LEFT:
            if(E.cx != 0)
                E.cx--;
            else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;

        case ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            }
            else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;

    }
    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

/*Calculating the size of the windows for display*/
int getWindowSize(int *rows, int *cols) {

    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return GetCursorPos(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }

}

/* Function for displaying the file row by row
 * Stores the text required into the text buffer*/
void drawRows(abuf* ab){
    int y;
    for(y = 0; y < E.screenrow; y++){
        int filerow = y + E.row_offset;
        if(filerow >= E.numrows){
            if (E.numrows == 0 && y == E.screenrow / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                        " Persistent text editor");
                if (welcomelen > E.screencol) welcomelen = E.screencol;
                int padding = (E.screencol - welcomelen) / 2;
                if (padding) {
                    appendAB(ab, "~", 1);
                    padding--;
                }
                while (padding--) appendAB(ab, " ", 1);
                appendAB(ab, welcome, welcomelen);
            }
            else {
                appendAB(ab, "~", 1);
            }
        }
        else{
            int len = E.row[filerow].rsize - E.col_offset;
            if (len < 0)
                len = 0;

            if(len > E.screencol) 
                len = E.screencol;
            char *c = &E.row[filerow].render[E.col_offset];
            int j;
            for (j = 0; j < len; j++) {
                if (isdigit(c[j])) {
                    appendAB(ab, "\x1b[33m", 5);
                    appendAB(ab, &c[j], 1);
                    appendAB(ab, "\x1b[39m", 5);
                } else {
                    appendAB(ab, &c[j], 1);
                }
            }
        }
        appendAB(ab, "\x1b[K", 3);
        appendAB(ab, "\r\n", 2);
    }
}

/*Opens the file passed to the function and displays the file after appending
 * it to the struct erow*/
void editorOpen(char* filename, int version){

    char* Version = NULL;
    char cache[] = "cache/";
    if(filename){
        strcpy(E.filename, filename);
    }
    else if((!filename)&&(E.first != 1)){
        char* temp = editorPrompt("Open: %s (ESC to cancel)");
        Version = editorPrompt("Version: %s");
        version = atoi(Version);
        strcpy(E.filename, temp);
        if(E.filename[0] == '\0'){ 
            editorSetStatusMessage("Open aborted");
            return;
        }
    }

    FILE * fp;
    int Newsize = 0;
    char* temBuf;
    char *temmp;

    if(version){
        Newsize = strlen(E.filename) + 3 + strlen(cache);
        temBuf = (char*)malloc(sizeof(char)*Newsize);
        if(Version){
            strcpy(temBuf, cache);
            temBuf = strcat(temBuf, Version);
        }
        else{
            strcpy(temBuf, cache);
            sprintf(temmp, "%d", version);
            temBuf = strcat(temBuf, temmp);
        }
        temBuf = strcat(temBuf, E.filename);
        editorSetStatusMessage("%s", temBuf);
        
        fp = fopen(temBuf, "r");
        E.first = 0;
        E.version = version;
        if(!fp){
            editorSetStatusMessage("Cannot open file");
            return;
        }
    }
    else if(!version){
        Newsize = strlen(E.filename) + strlen(cache);
        temBuf = (char*)malloc(sizeof(char)*Newsize);
        
        strcpy(temBuf, cache);
        temBuf = strcat(temBuf, E.filename);

        fp = fopen(temBuf, "r");
        E.first = 0;
        if(!fp){
            editorSetStatusMessage("Cannot open file");
            return;
        }
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    E.numrows = 0;
    while((linelen = getline(&line, &linecap, fp)) != -1){
        while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')){
            linelen--;  //Length of line excluding newline and carriage return
            editorRefreshScreen();
            insertRow(E.numrows, line, linelen);
        }
    }
    free(line);
    fclose(fp);

    E.dirty = 0;

}

/*insert the last row with required string*/
void insertRow(int at, char *s, size_t len){

    if(at < 0 || at > E.numrows) return;

    E.row = realloc(E.row, sizeof(erow)*(E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow)*( E.numrows - at));

    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editorUpdateRow(&E.row[at]);

    E.numrows++;
    E.dirty++;

}

/*Scroll past one page of the text displayed*/
void editorScroll() {
    E.rx = 0;
    if (E.cy < E.numrows) {
        E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    }
    if (E.cy < E.row_offset) {
        E.row_offset = E.cy;
    }
    if (E.cy >= E.row_offset + E.screenrow) {
        E.row_offset = E.cy - E.screenrow + 1;
    }
    if(E.cx < E.col_offset){
        E.col_offset = E.rx;
    }
    if(E.cx >= E.col_offset + E.screencol) {
        E.col_offset = E.rx - E.screencol + 1;
    }
}

/*Insert character in a row at a position*/
void rowInsertCharAt(erow* row, int at, char c){
    if(at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);

    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);

    E.dirty++;
}

/*Inserts the required character into the row*/
void insertChar(int c){
    if(E.cy == E.numrows){
        insertRow(E.numrows, "", 0);
    }
    rowInsertCharAt(&E.row[E.cy], E.cx, c);
    E.cx++;
    E.dirty++;
}

/* Copy contents from erow struct into a string*/
char* rowToString(int *buflen){
    int t = 0, j = 0;

    for(j = 0; j < E.numrows; j++){
        t += E.row[j].size + 1;
    }
    *buflen = t;

    char *buf = (char*) malloc(sizeof(char)*t);
    char *p = buf;

    for(j = 0; j < E.numrows; j++){
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }

    return buf;

}

/*Saving the newest version to file*/   
void editorSave(){

    if(E.filename[0] == '\0'){
        char* temp = editorPrompt("Save as: %s (ESC to cancel)");
        strcpy(E.filename, temp);
        if(E.filename[0] == '\0'){
            editorSetStatusMessage("Save aborted");
        }
    }

    int len; 
    char* buf = rowToString(&len);
    char filename[100];
    char temmp[3];
    char cache[] = "cache/";
    E.version+=1;

    /*Format for filename would be versionNumberFilename*/
    strcpy(filename, cache);
    sprintf(temmp, "%d", E.version);
    strcat(filename, temmp);
    strcat(filename, E.filename);
    editorSetStatusMessage("%s filename ", filename);

    int fd = open(filename, O_RDWR | O_CREAT, 0644);
    if(fd != -1) {
        if(ftruncate(fd, len) != -1){
            if(write(fd, buf, len) == len){
                close(fd);
                free(buf);
                E.dirty = 0;    
                editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
    return;
} 

/*Deleting a character*/
void deleteChar(erow *row, int at){
    if(at < 0 || at >= row -> size) return;
    memmove(&row -> chars[at], &row -> chars[at + 1], row -> size - at);
    row -> size --;
    editorUpdateRow(row);
    E.dirty++;
    return;
}

/*Implementation of deleteChar*/
void editorDelChar(){
    if(E.cy == E.numrows) return;
    if (E.cx == 0 && E.cy == 0) return;

    erow *row = &E.row[E.cy];
    if(E.cx > 0){
        deleteChar(row, E.cx - 1);
        E.cx--;
    }
    else {
        E.cx = E.row[E.cy - 1].size;
        editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        delRow(E.cy);
        E.cy--;
    }
}

/*Inserts a new line after pressing enter*/
void editorInsertNewline() {
    if (E.cx == 0) {
        insertRow(E.cy, "", 0);
    } 

    else {
        erow *row = &E.row[E.cy];
        insertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

/*Delete an entire row*/
void delRow(int at){
    if((at < 0) ||(at >= E.numrows)){
        return;
    }

    freeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    E.numrows--;
    E.dirty++;

}

void freeRow(erow *row){
    free(row -> chars);
    free(row->render);
    return;
}

void editorDrawStatusBar(struct abuf *ab) {
    appendAB(ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines",
            E.filename ? E.filename : "[No Name]", E.numrows);
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", E.cy + 1, E.numrows);

    if (len > E.screencol) len = E.screencol;
    appendAB(ab, status, len);

    while (len < E.screencol) {
        if (E.screencol - len == rlen) {
            appendAB(ab, rstatus, rlen);
            break;
        } else {
            appendAB(ab, " ", 1);
            len++;
        }
    }
    appendAB(ab, "\x1b[m", 3);
    appendAB(ab, "\r\n", 2);
}

void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

void editorDrawMessageBar(struct abuf *ab) {
    appendAB(ab, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);
    if (msglen > E.screencol) msglen = E.screencol;
    if (msglen && time(NULL) - E.statusmsg_time < 5)
        appendAB(ab, E.statusmsg, msglen);
}

void editorUpdateRow(erow *row) {

    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;
    free(row->render);
    row->render = malloc(row->size + tabs*7 + 1);

    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % 8 != 0) row->render[idx++] = ' ';
        }
        else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

char *editorPrompt(char *prompt) {

    size_t bufsize = 128;
    char *buf = malloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';

    while (1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();

        int c = editorReadKey();
        if (c == DEL_KEY || c == CTRL_key('h') || c == BACKSPACE) {
            if (buflen != 0) buf[--buflen] = '\0';
        }
        else if (c == '\x1b') {
            editorSetStatusMessage("");
            free(buf);
            return NULL;
        }
        else if (c == '\r') {
            if (buflen != 0) {
                editorSetStatusMessage("");
                return buf;
            }
        } 
        else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
    }
}

int editorRowCxToRx(erow *row, int cx) {
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (row->chars[j] == '\t')
            rx += 7 - (rx % 8);
        rx++;
    }
    return rx;
}
