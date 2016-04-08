#ifndef STATE_H
#define STATE_H

enum states {MENUMAIN, GAME, MAX_STATES};
enum events {INITALIZE, RUN, CLOSE, PAUSE, RESUME, SDLEVENT, MAX_EVENTS};

//STATE FUNCTIONS
void state_game_init(void *ptr);
void state_game_run(void *ptr);
void state_game_close(void *ptr);
void state_game_event(void *ptr);

void state_menu_init(void *ptr);
void state_menu_run(void *ptr);
void state_menu_close(void *ptr);
void state_menu_event(void *ptr);

//<<<<<<<<<<<<<<<<<<

extern void (*const statetable[MAX_STATES][MAX_EVENTS]) (void *ptr);

void state_queuepush(enum states state);
void state_queuepop();

void exitgame();

void updatewindowbounds(int w, int h);
void getwindowsize(int *w, int* h);
void swapwindow();
char *getbasepath();
void centermouse();

#endif
