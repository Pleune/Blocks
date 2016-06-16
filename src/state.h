#ifndef STATE_H
#define STATE_H

enum states {MENUMAIN, GAME, INVENTORY, MAX_STATES};
enum events {INITALIZE, RUN, CLOSE, PAUSE, RESUME, SDLEVENT, MAX_EVENTS};

//STATE FUNCTIONS
void state_game_init();
void state_game_run();
void state_game_close();
void state_game_event();
void state_game_pause();
void state_game_resume();

void state_menu_init();
void state_menu_run();
void state_menu_close();
void state_menu_event();

void state_inventory_init();
void state_inventory_run();
void state_inventory_close();
void state_inventory_event();

//<<<<<<<<<<<<<<<<<<

extern void (*const statetable[MAX_STATES][MAX_EVENTS]) (void *ptr);

void state_queue_push(enum states state);
void state_queue_pop();

inline static void
state_queue_switch(enum states state)
{
	state_queue_pop();
	state_queue_push(state);
}

int state_is_initalized(enum states state);

void state_exit();

void state_window_update(int w, int h);
void state_window_get_size(int *w, int* h);
void state_window_swap();

char *state_basepath_get();

void state_mouse_center();
int state_has_controller();

#endif
