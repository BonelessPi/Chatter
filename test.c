#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>

int main() {
    initscr();
    
    int key;
    while ((key = getch()) != 27) {
        if (key == KEY_RESIZE) {
            clear();
            if(COLS < 60 || LINES < 10){
                mvprintw(0, 0, "TERM TOO SMALL!!");
            }
            else{
                mvprintw(0, 0, "COLS = %d, LINES = %d", COLS, LINES);
            }
            for (int i = 0; i < COLS; i++)
                mvaddch(1, i, '*');
            refresh();
        }
    }

    endwin();
    return 0;
}