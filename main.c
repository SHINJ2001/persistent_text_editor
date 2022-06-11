#include "headers.h"

int main(int argc, char **argv){
    enableRawMode();
    init_editor();
    if(argc > 1){
        if(argv[2])
            editorOpen(argv[1], atoi(argv[2]));
        else
            editorOpen(argv[1], 0);
    }
    editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit");
    
    while(1){
        editorRefreshScreen();
        editorKeyPress();   
    }
    return 0;
}
