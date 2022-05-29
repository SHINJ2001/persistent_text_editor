#include "editor_interface.h"

struct termios orig_termios;

void enableRawMode() {

    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(IXON | ICRNL); //Prevents ctrl s and ctrl q from suspending program
    raw.c_oflag &= ~OPOST;
    raw.c_lflag &= ~(ECHO | 
            ICANON |  //Allows us to read input byte by byte instead of line by line and turns off echo
            ISIG); //Allows deactivating ctrl c and ctrl z commands

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIOME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios);
}

void disableRawMode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
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
        if(nread == -1) die("read");
    }

    return c;
}

void editorKeyPress(){
    char c = editorReadKey();
    
    switch(c) {
        case (CTRL_key('q')):
            editorRefreshScreen();
            exit(0);
            break;
    }
}

void editorRefreshScreen(){
    write(STDIN_FILENO, "\x1b[2J", 4);
    write(STDIN_FILENO, "\x1b{H", 3);

}

int GetCursorPos(int *rows, int* cols){

    char buf[32];
    int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    while(i < sizeof(buf) - 1){
        if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
        i++;
    }
    buf[i] = '\0';

    if(buf[0] != '\x1b' || buf[1]  != '[') return -1;
    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

