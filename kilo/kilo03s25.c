#include "../metaleap.c"
#include <errno.h>
#include <unistd.h>
#include <termios.h>

typedef struct termios Termios;
Termios orig_term_attrs;

#define KEY_CTRL(__the_key_code__) ((__the_key_code__)&0x1f)

// terminal

void termPosTopLeft() {
    write(1, "\x1b[H", 3); // position cursor at top-left corner
}

void termClearAndPosTopLeft() {
    write(1, "\x1b[2J", 4); // clear screen
    termPosTopLeft();       // position cursor at top-left corner
}

void fail(CStr const msg) {
    termClearAndPosTopLeft();
    perror(msg); // errno description
    abort();     // exit code 1
}

void termDisableRawMode() {
    tcsetattr(0, TCSAFLUSH, &orig_term_attrs); // restore terminal attrs
}

void termEnableRawMode() {
    if (-1 == tcgetattr(0, &orig_term_attrs))
        fail("tcgetattr");
    atexit(termDisableRawMode);

    Termios term_attrs = orig_term_attrs;
    term_attrs.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    term_attrs.c_oflag &= ~OPOST;
    term_attrs.c_cflag |= CS8;
    term_attrs.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    term_attrs.c_cc[VMIN] = 0;
    term_attrs.c_cc[VTIME] = 1;

    if (-1 == tcsetattr(0, TCSAFLUSH, &term_attrs))
        fail("tcsetattr");
}

char termReadKey() {
    char key = 0;
    while (key == 0) {
        int n_read = read(0, &key, 1);
        if (n_read == -1 && errno != EAGAIN)
            fail("stdin read error");
        if (n_read <= 0)
            key = 0;
    }
    return key;
}

// input

void edProcessInput() {
    char key = termReadKey();
    switch (key) {
        case KEY_CTRL('q'): {
            termClearAndPosTopLeft();
            exit(0);
        } break;
    }
}

// output

void edDrawRows() {
    for (int y = 0; y < 24; y += 1)
        write(1, "~\r\n", 3);
}

void edRefreshScreen() {
    termClearAndPosTopLeft();
    edDrawRows();
    termPosTopLeft();
}

// init

int main() {
    termEnableRawMode();
    while (true) {
        edRefreshScreen();
        edProcessInput();
    }
    return 0;
}
