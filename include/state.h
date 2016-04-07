#ifndef STATE_H
#define STATE_H

enum states {MENUMAIN, GAME, MAX_STATES};
enum events {INITALIZE, RUN, CLOSE, PAUSE, RESUME, MAX_EVENTS};

//STATE FUNCTIONS

void state_game_init();
void state_game_run();
void state_game_close();


void state_menu_init();
void state_menu_run();
void state_menu_close();

//<<<<<<<<<<<<<<<<<<

extern void (*const statetable[MAX_STATES][MAX_EVENTS]) (void);

void state_queuepush(enum states state);
void state_queuepop();

void exitgame();

void updatewindowbounds(int w, int h);
void getwindowsize(int *w, int* h);
void swapwindow();
char *getbasepath();
void centermouse();

#endif
