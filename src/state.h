#ifndef STATE_H
#define STATE_H

#include <stdint.h>

enum states {MENUMAIN, WORLD_LOAD, WORLD_NEW, GAME, INVENTORY, MAX_STATES};
enum events {INITALIZE, RUN, CLOSE, PAUSE, RESUME, SDLEVENT, MAX_EVENTS};

//<<<<<<<<<<<<<<<<<<

//STATE FUNCTIONS
void state_game_init(void *ptr);
void state_game_run(void *ptr);
void state_game_close(void *ptr);
void state_game_event(void *ptr);
void state_game_pause(void *ptr);
void state_game_resume(void *ptr);

void state_world_load(void *ptr);
void state_world_new(void *ptr);
void state_world_loop(void *ptr);
void state_world_cleanup(void *ptr);

void state_menu_init(void *ptr);
void state_menu_run(void *ptr);
void state_menu_close(void *ptr);
void state_menu_event(void *ptr);

void state_inventory_init(void *ptr);
void state_inventory_run(void *ptr);
void state_inventory_close(void *ptr);
void state_inventory_event(void *ptr);

extern void (*const statetable[MAX_STATES][MAX_EVENTS]) (void *ptr);

void state_queue_push(enum states state, void *ptr);
void state_queue_pop();
void state_queue_fail(); //exits state w/o cleanup

inline static void
state_queue_switch(enum states state, void *ptr)
{
	state_queue_pop();
	state_queue_push(state, ptr);
}

int state_is_initalized(enum states state);

void state_exit();

void state_window_update(int w, int h);
void state_window_get_size(int *w, int* h);
void state_window_swap();
uint32_t state_window_get_id();

const char *state_basepath_get();
const char *state_prefpath_get();

void state_mouse_center();
int state_has_controller();

#endif
