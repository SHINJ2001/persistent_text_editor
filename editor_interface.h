#ifndef EDITOR_INTERFACE_H
#define EDITOR_INTERFACE_H

#include "headers.h"

#define CTRL_key(k) ((k) & 0x1f)

typedef struct editor_config{
    int cx, cy;
    int screenrow, screencol;
    struct termios orig_termios;
} editor_config;

enum editorkey{
    ARROW_LEFT = 1000,
    ARROW_RIGHT, 
    ARROW_UP,
    ARROW_DOWN
};

void enableRawMode();
void disableRawMode();
void die(const char*);
char editorReadKey();
void editorKeyPress();
void editorRefreshScreen();
int GetCursorPos(int *rows, int* cols);
void moveCursor(char c);
int getWindowSize(int *rows, int* cols);
void drawRows(abuf*);
#endif
