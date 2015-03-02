#ifndef STATE_H
#define STATE_H

enum states {MENU, GAME, STARTING, CLOSING, MAX_STATES};

void state_changeto(enum states newstate);

void updatewindowbounds(int w, int h);
void getwindowsize(int *w, int* h);
void swapwindow();
char *getbasepath();
void centermouse();

#endif
