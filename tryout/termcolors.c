#include "../utils_libc_deps_basics.c"

void tryTermColors() {
    UInt const num_cols = 191;
    UInt const num_rows = 36;

    for (UInt row = 0; row < num_rows; row += 1)
        for (UInt col = 0; col < num_cols; col += 1) {
            U8 r = col, g = (127 - row) + (127 - col), b = 255 - row;
            printf("\x1b[38;2;%hhu;%hhu;%hhum█\x1b[0m", r, g, b);
        }
}
