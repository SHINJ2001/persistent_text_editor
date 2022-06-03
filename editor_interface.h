#ifndef EDITOR_INTERFACE_H
#define EDITOR_INTERFACE_H

#include "headers.h"

#define CTRL_key(k) ((k) & 0x1f)

typedef struct erow{
    int size; //For storing rows of text stored
    char *chars;
} erow;

typedef struct editor_config{
    int cx, cy;
    int screenrow, screencol;
    int numrows;
    erow *row; //pointer so that multiple rows can be stored
    int row_offset; //Keep track of what row user is at while scrolling
    int col_offset; //Keep track of what column user is at while scrolling
    char *filename;
    int dirty; //Check for any unsaved changes 
    struct termios orig_termios;
} editor_config;

enum editorkey{
    BACKSPACE = 127, 
    ARROW_LEFT = 1000,
    ARROW_RIGHT , 
    ARROW_UP ,
    ARROW_DOWN, 
    DEL_KEY
};

void init_editor();
void enableRawMode(void);
void disableRawMode(void);
void die(const char*);
int editorReadKey(void);
void editorKeyPress(void);
void editorRefreshScreen(void);
int GetCursorPos(int *rows, int* cols);
void moveCursor(int c);
int getWindowSize(int *rows, int* cols);
void drawRows(abuf*);
void editorOpen(char*);
void insertRow(int, char*, size_t);
void editorScroll(void);
void editorRowInsertChar(erow* row, int at, int c);
void insertChar(int c);
char* rowToString(int *buflen);
void editorSave(void);
void editorDelChar(void);
void deleteChar(erow *row, int at);

#endif
