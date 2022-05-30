#include "headers.h"

int main(){
    enableRawMode();
    init_editor();
    while(1){
        editorKeyPress();   
        editorRefreshScreen();
    }
    return 0;
}
