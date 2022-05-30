#include "editor_interface.h"

editor_config E;
void init_editor(){
    E.cx = 0; //Cursor positions
    E.cy = 0;

    E.numrows = 0; //Number of editor rows
    if(getWindowSize(&E.screenrow, &E.screencol) == -1) die("getWindowSize");// Writing window size to  struct

}

void enableRawMode() {

    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | IXON | ICRNL | INPCK | ISTRIP ); //Prevents ctrl s and ctrl q from suspending program
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
    while((nread = read(STDIN_FILENO, &c, 1))!= 1){
        if((nread == -1)&&(errno != EAGAIN)) die("read");
    }

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
        else{
            return c; //Return char if not a control sequence
        }

    }
}

void editorKeyPress(){
    int c = editorReadKey();

    switch(c) {
        case (CTRL_key('q')):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
            moveCursor(c);
            break;
    }
}

void editorRefreshScreen(){
    abuf ab;
    initAbuf(&ab);

    appendAB(&ab, "\x1b[?25l", 6);
    appendAB(&ab, "\x1b[H", 3);
    drawRows(&ab);

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
    appendAB(&ab, buffer, strlen(buffer));

    appendAB(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    freeAB(&ab);
}

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
            if(E.cy != E.screenrow - 1)
                E.cy++;
            break;
        case ARROW_LEFT:
            if(E.cx != 0)
                E.cx--;
            break;
        case ARROW_RIGHT:
            if(E.cx != E.screencol - 1)
                E.cx++;
            break;
    }
}
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

void drawRows(abuf* ab){
    for(int y = 0; y < E.screenrow; y++){
        if(y >= E.numrows){
            appendAB(ab, "~", 1);
        }
        else{
            int len = E.row.size;
            if(len > E.screencol) len = E.screencol;
            appendAB(ab, E.row.chars, len);
        }
        appendAB(ab, "\x1b[K", 3);
        if(y < E.screenrow - 1){
            appendAB(ab, "\r\n", 2);
        }
    }
}

void editorOpen(char* filename){

    FILE *fp = fopen(filename, "r");
    if(!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen = getline(&line, &linecap, fp);

    if(linelen != -1){
        while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;  //Length of line excluding newline and carriage return
    
        E.row.size = linelen;
        E.row.chars = (char*)malloc(linelen + 1);
        memcpy(E.row.chars, line, linelen);
        E.row.chars[linelen] = '\0';
        E.numrows = 1;
    }
    free(line);
    fclose(fp);

}

