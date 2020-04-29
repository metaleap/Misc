#include "../metaleap.c"
#include <unistd.h>
#include <termios.h>

typedef struct termios Termios;


Termios orig_term_attrs;


void disableRawMode() {
    if (-1 == tcsetattr(0, TCSAFLUSH, &orig_term_attrs))
        ·fail(str("failed to tcsetattr"));
}


void enableRawMode() {
    if (-1 == tcgetattr(0, &orig_term_attrs))
        ·fail(str("failed to tcgetattr"));
    atexit(disableRawMode);

    Termios term_attrs = orig_term_attrs;
    term_attrs.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    term_attrs.c_oflag &= ~OPOST;
    term_attrs.c_cflag |= CS8;
    term_attrs.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    term_attrs.c_cc[VMIN] = 0;
    term_attrs.c_cc[VTIME] = 1;

    if (-1 == tcsetattr(0, TCSAFLUSH, &term_attrs))
        ·fail(str("failed to tcsetattr"));
}


int main() {
    enableRawMode();
    while (true) {
        int chr = 0;
        read(0, &chr, 1);
        if (chr == 17)
            exit(0);
        if (chr > 0)
            printf("%d\r\n", chr);
    }
}
