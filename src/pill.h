#ifndef PILL_H
#define PILL_H

#include <termios.h>

/* Defines */

#define PILL_VERSION "1"

#define CTRL_KEY(k) ((k) & 0x1f)

enum ARROW_KEYS{
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DELETE_KEY,
    HOME_KEY,
    END_KEY, 
    PAGE_UP,
    PAGE_DOWN,
};

/* Data */

struct edRow{
    int sz;
    char* chars;
};

struct config{
    int rows;
    int cols;
    struct termios orig_termios;
    int cx, cy;
    int numRows;
    struct edRow* row;
};



/* Termio */

void die(const char* s);

void disableRawMode();

void enableRawMode();

int readKey();

int windowSize(int* rows, int* cols);

int cursorPos(int* rows, int* cols);

/* Row Methods */

void edAppendRow(char* s, size_t len);

/* File I/O */

void edOpen(char* filename);

/* Append Buffer */

struct abuf{
    char* b;
    unsigned int len;
};

#define INIT_ABUF {NULL, 0}

/* Input */

void processKeyPress();

void moveCursor(int key);

/* Output */

void refreshScreen();

void drawRows(struct abuf* ab);

/* Init */

void startEditor();

#endif //PILL_H