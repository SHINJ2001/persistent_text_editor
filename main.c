#include "headers.h"

int main(){
    enableRawMode();
    while(1){
        editorKeyPress();   
        editorRefeshScreen();
    }
    return 0;
}
