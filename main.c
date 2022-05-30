#include "headers.h"

int main(int argc, char **argv){
    enableRawMode();
    init_editor();
    if(argc > 1){
        editorOpen(argv[1]);
    }
    while(1){
        editorRefreshScreen();
        editorKeyPress();   
    }
    return 0;
}
