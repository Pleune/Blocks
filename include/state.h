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

void state_queue_push(enum states state);
void state_queue_pop();

void state_exit();

void state_window_update(int w, int h);
void state_window_get_size(int *w, int* h);
void state_window_swap();

char *state_basepath_get();

void state_mouse_center();
int state_has_controller();

#endif
