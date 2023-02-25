#ifndef EDITOR_INTERFACE_H
#define EDITOR_INTERFACE_H

#include "headers.h"

#define CTRL_key(k) ((k) & 0x1f)

typedef struct erow{
    int size; //For storing rows of text stored
    char *chars;
    int rsize;
    char *render;
} erow;

typedef struct editor_config{
    int cx, cy;
    int screenrow, screencol;
    int numrows;
    erow *row; //pointer so that multiple rows can be stored
    int row_offset; //Keep track of what row user is at while scrolling
    int col_offset; //Keep track of what column user is at while scrolling
    char filename[100];
    int version;
    int dirty; //Check for any unsaved changes 
    struct termios orig_termios;
    char statusmsg[80];
    time_t statusmsg_time;
    int rx;
    int first;
    int count;

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
void editorOpen(char* filename, int version);
void insertRow(int, char*, size_t);
void editorScroll(void);
void editorRowInsertChar(erow* row, int at, int c);
void insertChar(int c);
char* rowToString(int *buflen);
void editorSave(void);
void editorDelChar(void);
void deleteChar(erow *row, int at);
void editorInsertNewline(void);
void delRow(int at);
void freeRow(erow *row);
void editorDrawStatusBar(struct abuf *ab);
void editorSetStatusMessage(const char *fmt, ...);
void editorDrawMessageBar(abuf *ab);
void editorUpdateRow(erow *row);
void editorRowAppendString(erow *row, char *s, size_t len);
char *editorPrompt(char *prompt);
int editorRowCxToRx(erow *row, int cx);

#endif
