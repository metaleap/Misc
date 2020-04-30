#include "../metaleap.c"
#include <errno.h>
#include <unistd.h>
#include <termios.h>

typedef struct termios Termios;
Termios orig_term_attrs;

#define KEY_CTRL(__the_key_code__) ((__the_key_code__)&0x1f)

// terminal

void fail(CStr const msg) {
    perror(msg);
    abort();
}

void disableRawMode() {
    if (-1 == tcsetattr(0, TCSAFLUSH, &orig_term_attrs))
        fail("failed to tcsetattr");
}

void enableRawMode() {
    if (-1 == tcgetattr(0, &orig_term_attrs))
        fail("failed to tcgetattr");
    atexit(disableRawMode);

    Termios term_attrs = orig_term_attrs;
    term_attrs.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    term_attrs.c_oflag &= ~OPOST;
    term_attrs.c_cflag |= CS8;
    term_attrs.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    term_attrs.c_cc[VMIN] = 0;
    term_attrs.c_cc[VTIME] = 1;

    if (-1 == tcsetattr(0, TCSAFLUSH, &term_attrs))
        fail("failed to tcsetattr");
}

char edReadKey() {
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

void edProcessKeypress() {
    char key = edReadKey();
    switch (key) {
        case KEY_CTRL('q'): {
            exit(0);
        } break;
    }
}

// output

void edRefreshScreen() {
    write(1, "\x1b[2J", 4);
}

// init

int main() {
    enableRawMode();
    while (true) {
        edRefreshScreen();
        edProcessKeypress();
    }
    return 0;
}
