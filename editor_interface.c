#include "headers.h"

editor_config E;
void init_editor(){
    E.cx = 0; //Cursor positions
    E.cy = 0;

    E.numrows = 0; //Number of editor rows
    E.row = NULL;
    E.row_offset = 0;
    E.col_offset = 0;
    E.dirty = 0;

    if(getWindowSize(&E.screenrow, &E.screencol) == -1) die("getWindowSize");// Writing window size to  struct

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
    int quit = 2;

    switch(c) {
        case '\r':
            editorInsertNewline();
            break;

        case (CTRL_key('q')):
            if(E.dirty && quit > 0){
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

        case CTRL_key('s'):
            editorSave();
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

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "\x1b[%d;%dH", (E.cy - E.row_offset) + 1, E.cx + 1);
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
            break;
        case ARROW_RIGHT:
                E.cx++;
            break;
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
            appendAB(ab, "~", 1);
        }
        else{
            int len = E.row[filerow].size - E.col_offset;
            if (len < 0)
                len = 0;

            if(len > E.screencol) 
                len = E.screencol;
            appendAB(ab, &E.row[filerow].chars[E.col_offset], len);
        }
        appendAB(ab, "\x1b[K", 3);
        if(y < E.screenrow - 1){
            appendAB(ab, "\r\n", 2);
        }
    }
}

/*Opens the file passed to the function and displays the file after appending
 * it to the struct erow*/
void editorOpen(char* filename){

    free(E.filename);
    E.filename = strdup(filename);
    FILE *fp = fopen(filename, "r");
    if(!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while((linelen = getline(&line, &linecap, fp)) != -1){
        while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')){

            linelen--;  //Length of line excluding newline and carriage return
            insertRow(E.numrows, line, linelen);
        }
    }
    free(line);
    fclose(fp);
    
    E.dirty = 0;
}
/* Append the last row with required string*/
void insertRow(int at, char *s, size_t len){

    if(at < 0 || at > E.numrows) return;

    E.row = realloc(E.row, sizeof(erow)*(E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow)* E.numrows - at);

    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.numrows += 1;

    E.dirty++;

}

/*Scroll past one page of the text displayed*/
void editorScroll() {
  if (E.cy < E.row_offset) {
    E.row_offset = E.cy;
  }
  if (E.cy >= E.row_offset + E.screenrow) {
    E.row_offset = E.cy - E.screenrow + 1;
  }
  if(E.cx < E.col_offset){
      E.col_offset = E.cx;
  }
  if(E.cx >= E.col_offset + E.screencol) {
      E.col_offset = E.cx - E.screencol + 1;
  }
}
/*Insert character in a row at a position*/
void rowInsertCharAt(erow* row, int at, char c){
    if(at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    
    row->size++;
    row->chars[at] = c;

    E.dirty++;
}
/*Inserts the required character into the row*/
void insertChar(int c){
    if(E.cy == E.numrows){
        insertRow(E.numrows, "", 0);
    }
    rowInsertCharAt(&E.row[E.cy], E.cx, c);
    E.cx++;
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
    
void editorSave(){
    
    if(E.filename == NULL)
        return;

    int len; 
    char* buf = rowToString(&len);

    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if(fd != -1) {
        if(ftruncate(fd, len) != -1){
            if(write(fd, buf, len) == len){
                    close(fd);
                    free(buf);

                    E.dirty = 0;
            }
        }
        close(fd);
    }

    free(buf);
} 

/*Deleting a character*/
void deleteChar(erow *row, int at){
    if(at < 0 || at >= row -> size) return;
    memmove(&row -> chars[at], &row -> chars[at + 1], row -> size - at);
    row -> size --;
    E.dirty++;
}

/*Implementation of deleteChar*/
void editorDelChar(){
    if(E.cy == E.numrows) return;

    erow *row = &E.row[E.cy];
    if(E.cx > 0){
        deleteChar(row, E.cx - 1);
        E.cx--;
    }
}

void editorInsertNewline() {
  if (E.cx == 0) {
    insertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    insertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
  }
  E.cy++;
  E.cx = 0;
}
