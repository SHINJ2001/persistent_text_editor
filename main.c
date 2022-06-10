#include "headers.h"

int main(int argc, char **argv){
    enableRawMode();
    init_editor();
    if(argc > 1){
        editorOpen(argv[1], atoi(argv[2]));
    }
    editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit");
    
    while(1){
        editorRefreshScreen();
        editorKeyPress();   
    }
    return 0;
}
