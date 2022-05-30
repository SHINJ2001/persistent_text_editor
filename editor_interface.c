#include "editor_interface.h"

editor_config E;
void init_editor(){
    E.cx = 0; //Cursor positions
    E.cy = 0;

    if(getWindowSize(&E.screenrow, &E.screencol) == -1) die("getWindowSize");

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

char editorReadKey(){
    int nread;
    char c;
    while(nread = read(STDIN_FILENO, &c, 1 != 1)){
        if((nread == -1)&&(errno != EAGAIN)) die("read");
    }

    if(c == '\x1b'){
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if(seq[0] == '['){
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

void editorKeyPress(){
    char c = editorReadKey();
    
    switch(c) {
        case (CTRL_key('q')):
            exit(1);
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
    appendAB(&ab, "\x1b[H", 3);
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

void moveCursor(char key){
    switch(key){
        case ARROW_UP:
            E.cy--;
            break;
        case ARROW_DOWN:
            E.cy++;
            break;
        case ARROW_LEFT:
            E.cx--;
            break;
        case ARROW_RIGHT:
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
        appendAB(ab, "~", 3);
        appendAB(ab, "\x1b[K", 3);
        if(y < E.screenrow - 1){
            appendAB(ab, "\r\n", 2);
        }
    }
}

