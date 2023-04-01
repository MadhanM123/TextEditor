/* Includes */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include "pill.h"

/* Data */

struct config C;

/* Termio */

void die(const char* s){
    refreshScreen();

    perror(s);
    exit(1);
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &C.orig_termios) == -1) 
        die("tcsettattr");
}

void enableRawMode(){
    if(tcgetattr(STDIN_FILENO, &C.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = C.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char readKey(){
    int nread;
    char c;

    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("read");
    }

    return c;
}

int windowSize(int* rows, int* cols){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return cursorPos(rows, cols);
    }
    else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

int cursorPos(int* rows, int* cols){
    char buffer[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while(i < sizeof(buffer) - 1){
        if(read(STDIN_FILENO, &buffer[i], 1) != 1) break;
        if(buffer[i] == 'R') break;
        i++;
    }

    buffer[i] = '\0';

    if(buffer[0] != '\x1b' || buffer[1] == '[') return -1;
    if(sscanf(&buffer[2], "%d;%d", rows, cols) != 2) return -2;

    printf("\r\n&buffer[1]: '%s'\r\n", &buffer[1]);

    readKey();
    return -1;
}

/* Append Buffer */

void append(struct abuf* ab, const char* s, int len){
    
}

/* Input */

void processKeyPress(){
    char c = readKey();

    switch(c){
        case CTRL_KEY('q'):
            refreshScreen();
            exit(0);
            break;
    }
}

/* Output */

void refreshScreen(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3); //Moves cursor to home position

    drawRows();

    write(STDOUT_FILENO, "\x1b[H", 3);
}

void drawRows(){
    for(int y = 0; y < C.rows; y++){
        write(STDOUT_FILENO, "~", 1);

        if(y < C.rows - 1){
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }
}


/* Init */

void startEditor(){
    if(windowSize(&C.rows, &C.cols) == -1) die("windowSize");
}

int main(){
    printf("PILL IS RUNNING");

    enableRawMode();
    startEditor();

    while(1){
        refreshScreen();
        processKeyPress();
    }

    return 0;
}