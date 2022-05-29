#ifndef EDITOR_INTERFACE_H
#define EDITOR_INTERFACE_H

#include "headers.h"

#define CTRL_key(k) ((k) & 0x1f)
void enableRawMode();
void disableRawMode();
void die(const char*);
char editorReadKey();
void editorKeyPress();
void editorRefreshScreen();
int GetCursorPos(int *rows, int* cols);
