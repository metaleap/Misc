#include "../metaleap.c"
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

typedef struct termios Termios;
struct {
    Termios orig_term_attrs;
    struct {
        int rows;
        int cols;
    } screen;
} Ed;

#define KEY_CTRL(__the_key_code__) ((__the_key_code__)&0x1f)

// terminal

void termPosTopLeft() {
    write(1, "\x1b[H", 3); // position cursor at top-left corner
}

void termClearAndPosTopLeft() {
    write(1, "\x1b[2J", 4); // clear screen
    termPosTopLeft();
}

void fail(CStr const msg) {
    termClearAndPosTopLeft();
    perror(msg); // errno description
    abort();     // exit code 1
}

int termDimensions(int* rows, int* cols) {
    struct winsize win_size = {.ws_row = 0, .ws_col = 0};
    if (-1 == ioctl(1, TIOCGWINSZ, &win_size) || win_size.ws_row == 0 || win_size.ws_col == 0)
        return -1;
    *rows = win_size.ws_row;
    *cols = win_size.ws_col;
    return 0;
}

void termDisableRawMode() {
    tcsetattr(0, TCSAFLUSH, &Ed.orig_term_attrs); // restore terminal attrs
}

void termEnableRawMode() {
    if (-1 == tcgetattr(0, &Ed.orig_term_attrs))
        fail("tcgetattr");
    atexit(termDisableRawMode);

    Termios term_attrs = Ed.orig_term_attrs;
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
    U8 buf[3] = "\r\nL";
    write(1, "~", 1);
    for (int y = 0; y < Ed.screen.rows - 1; y += 1) {
        buf[2] = y + 65;
        write(1, buf, 3);
    }
}

void edRefreshScreen() {
    termClearAndPosTopLeft();
    edDrawRows();
    termPosTopLeft();
}

// init

void edInit() {
    if (-1 == termDimensions(&Ed.screen.rows, &Ed.screen.cols))
        fail("termDimensions");
}

int main() {
    termEnableRawMode();
    edInit();
    while (true) {
        edRefreshScreen();
        edProcessInput();
    }
    return 0;
}
