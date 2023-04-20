/* Includes */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#include <string.h>
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

int readKey(){
    int nread;
    char c;

    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("read");
    }

    if(c == '\x1b'){
        char seq[3];

        if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if(seq[0] == '['){
            if(seq[1] >= '0' && seq[1] <= '9'){
                if(read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if(seq[2] == '~'){
                    switch(seq[1]){
                        case '1': return HOME_KEY;
                        case '3': return DELETE_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }
            else{
                switch(seq[1]){
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        }
        else if(seq[0] == 'O'){
            switch(seq[1]){
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
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

void bufferAppend(struct abuf* ab, const char* s, int len){
    char* new = realloc(ab->b, ab->len + len);

    if(new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void bufferFree(struct abuf* ab){
    free(ab->b);
}

/* Input */

void moveCursor(int key){
    switch(key){
        case ARROW_UP:
            if(C.cy != 0) C.cy--;
            break;
        case ARROW_LEFT:
            if(C.cx != 0) C.cx--;
            break;
        case ARROW_DOWN:
            if(C.cy != C.rows - 1) C.cy++;
            break;
        case ARROW_RIGHT:
            if(C.cx != C.cols - 1) C.cx++;
            break;
    }
}

void processKeyPress(){
    int c = readKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case HOME_KEY:
            C.cx = 0;
            break;
        case END_KEY:
            C.cx = C.cols - 1;
            break;
        

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int scroll = C.rows;
                while(scroll--){
                    moveCursor(PAGE_UP ? ARROW_UP : ARROW_DOWN);
                }
            }
            break;

        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_RIGHT:
        case ARROW_DOWN:
            moveCursor(c);
            break;
    }
}

/* Output */

void refreshScreen(){
    struct abuf ab = INIT_ABUF;

    bufferAppend(&ab, "\x1b[?25l", 6);

    bufferAppend(&ab, "\x1b[H", 3); // Moves cursor to home position

    drawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", C.cy + 1, C.cx + 1);
    bufferAppend(&ab, buf, strlen(buf));

    bufferAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
}

void drawRows(struct abuf* ab){
    for(int y = 0; y < C.rows; y++){
        
        if(y == C.rows / 3){
            char welcomeMsg[80];
            int msglen = snprintf(welcomeMsg, sizeof(welcomeMsg), "Pill editor -- version %s", PILL_VERSION);
            msglen = msglen > C.cols ? C.cols : msglen;
            int pad = (C.cols  - msglen) / 2;
            if(pad){
                bufferAppend(ab, "~", 1);
                pad--;
            }
            while(pad--) bufferAppend(ab, " ", 1);
            bufferAppend(ab, welcomeMsg, msglen);
        }
        else{
            bufferAppend(ab, "~", 1);
        }
    
        bufferAppend(ab, "\x1b[K", 3);
        if(y < C.rows - 1){
            bufferAppend(ab, "\r\n", 2);
        }
    }
}


/* Init */

void startEditor(){
    C.cx = 0;
    C.cy = 0;
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