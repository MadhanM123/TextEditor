/*** Includes ***/

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

/*** Data ***/

struct termios orig_termios;

/*** Termio ***/

void die(const char* s){
    perror(s);
    exit(1);
}

void disableRawMode(){
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    die("tcsettattr");
}

void enableRawMode(){
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}


/*** Init ***/

int main(){
    printf("PILL IS RUNNING");

    enableRawMode();

    char c;
    while(1){
        c = '\0';
        if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
        if(iscntrl(c)){
            printf("%d\r\n", c);
        }
        else{
            printf("%d ('%c')\r\n", c, c);
        }

        if(c == 'q') break;
    }

    return 0;
}