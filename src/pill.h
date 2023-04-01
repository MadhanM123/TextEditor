#ifndef PILL_H
#define PILL_H

/* Defines */

#define CTRL_KEY(k) ((k) & 0x1f)

/* Data */

struct config{
    int rows;
    int cols;
    struct termios orig_termios;
};

/* Termio */

void die(const char* s);

void disableRawMode();

void enableRawMode();

char readKey();

int windowSize(int* rows, int* cols);

int cursorPos(int* rows, int* cols);

/* Append Buffer */

struct abuf{
    char* b;
    unsigned int len;
};

#define INIT_ABUF {NULL, 0}

/* Input */

void processKeyPress();

/* Output */

void refreshScreen();

void drawRows();

/* Init */

void startEditor();

#endif //PILL_H