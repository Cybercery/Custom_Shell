#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define MAX_LINES 1024
#define MAX_LEN 256

struct termios orig_termios;

int cur_line = 0, cur_col = 0;
int num_lines = 0;
char buffer[MAX_LINES][MAX_LEN];
char filename[256];
int text_style = 0;  // 0 = normal, 1 = bold, 2 = dim
int text_color = 37; // ANSI white default

/* ---------------- Terminal Setup ---------------- */
void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_iflag &= ~(IXON | ICRNL);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/* ---------------- Utility ---------------- */
void clear_screen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
    fflush(stdout);
}

/* ---------------- UI Drawing ---------------- */
void draw_static_ui() {
    clear_screen();
    printf("\033[1;34m--- CustomShell Text Editor ---\033[0m\n");
    printf("\033[33m(Ctrl+S=Save, Ctrl+Q/Ctrl+C=Quit, Ctrl+F=Font, Ctrl+R=Color)\033[0m\n");
    printf("\033[36m(Arrow Keys=Move Cursor, Enter=New Line, Backspace=Delete)\033[0m\n\n");
}

void redraw_text() {
    printf("\033[5;1H"); // position cursor at start of text area
    printf("\033[%d;%dm",
        text_style == 1 ? 1 : (text_style == 2 ? 2 : 0),
        text_color);

    for (int i = 0; i < num_lines; i++) {
        printf("\033[K%s\n", buffer[i]);  // clear line before writing
    }

    printf("\033[0m"); // reset style
    move_cursor(cur_line + 5, cur_col + 1);
    fflush(stdout);
}

/* ---------------- File Operations ---------------- */
void save_file() {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("\033[1;31mError: could not save file!\033[0m\n");
        return;
    }
    for (int i = 0; i < num_lines; i++) {
        fprintf(fp, "%s\n", buffer[i]);
    }
    fclose(fp);
    printf("\033[1;32m\n[Saved successfully]\033[0m\n");
    fflush(stdout);
}

/* ---------------- Input Handling ---------------- */
void handle_arrow(char c) {
    if (c == 'A' && cur_line > 0) cur_line--;                    // Up
    else if (c == 'B' && cur_line < num_lines - 1) cur_line++;   // Down
    else if (c == 'C' && cur_col < strlen(buffer[cur_line])) cur_col++; // Right
    else if (c == 'D' && cur_col > 0) cur_col--;                 // Left
}

void toggle_font() {
    text_style = (text_style + 1) % 3;
    redraw_text();
}

void toggle_color() {
    int colors[] = {31, 32, 33, 34, 35, 36, 37};
    static int idx = 6;
    idx = (idx + 1) % 7;
    text_color = colors[idx];
    redraw_text();
}

/* ---------------- Main ---------------- */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: textedit <filename>\n");
        return 1;
    }

    strcpy(filename, argv[1]);

    // Load file if exists
    FILE *fp = fopen(filename, "r");
    if (fp) {
        while (fgets(buffer[num_lines], MAX_LEN, fp)) {
            buffer[num_lines][strcspn(buffer[num_lines], "\n")] = '\0';
            num_lines++;
        }
        fclose(fp);
    } else {
        num_lines = 1;
        buffer[0][0] = '\0';
    }

    enableRawMode();
    draw_static_ui();
    redraw_text();

    while (1) {
        char c;
        read(STDIN_FILENO, &c, 1);

        if (c == 27) { // Arrow keys
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) == 0) continue;
            if (seq[0] == '[')
                handle_arrow(seq[1]);
        }
        else if (c == CTRL_KEY('q') || c == CTRL_KEY('c')) {
            printf("\033[1;31m\n[Exiting editor...]\033[0m\n");
            break;
        }
        else if (c == CTRL_KEY('s')) {
            save_file();
        }
        else if (c == CTRL_KEY('f')) {
            toggle_font();
        }
        else if (c == CTRL_KEY('r')) {
            toggle_color();
        }
        /* --- Enter: split line --- */
        else if (c == '\n') {
            if (num_lines < MAX_LINES - 1) {
                int len = strlen(buffer[cur_line]);
                if (cur_col > len) cur_col = len;

                for (int i = num_lines; i > cur_line; i--)
                    strcpy(buffer[i], buffer[i - 1]);

                memmove(buffer[cur_line + 1], &buffer[cur_line][cur_col],
                        strlen(&buffer[cur_line][cur_col]) + 1);

                buffer[cur_line][cur_col] = '\0';

                num_lines++;
                cur_line++;
                cur_col = 0;
            }
        }
        /* --- Backspace: delete or merge --- */
        else if (c == 127 || c == '\b') {
            if (cur_col > 0) {
                memmove(&buffer[cur_line][cur_col - 1],
                        &buffer[cur_line][cur_col],
                        strlen(&buffer[cur_line][cur_col]) + 1);
                cur_col--;
            } else if (cur_line > 0) {
                int prev_len = strlen(buffer[cur_line - 1]);
                int cur_len = strlen(buffer[cur_line]);
                if (prev_len + cur_len < MAX_LEN - 1) {
                    strcat(buffer[cur_line - 1], buffer[cur_line]);
                    for (int i = cur_line; i < num_lines - 1; i++)
                        strcpy(buffer[i], buffer[i + 1]);
                    buffer[num_lines - 1][0] = '\0';
                    num_lines--;
                    cur_line--;
                    cur_col = prev_len;
                }
            }
        }
        else if (isprint(c)) {
            int len = strlen(buffer[cur_line]);
            if (len < MAX_LEN - 1) {
                memmove(&buffer[cur_line][cur_col + 1],
                        &buffer[cur_line][cur_col],
                        len - cur_col + 1);
                buffer[cur_line][cur_col] = c;
                cur_col++;
            }
        }

        redraw_text();
    }

    disableRawMode();
    clear_screen();
    printf("Exited editor.\n");
    return 0;
}
