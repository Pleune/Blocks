#include "state.h"

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

void state_menu_init(void *ptr);
void state_menu_run(void *ptr);
void state_menu_close(void *ptr);
void state_menu_event(void *ptr);

void state_inventory_init(void *ptr);
void state_inventory_run(void *ptr);
void state_inventory_close(void *ptr);
void state_inventory_event(void *ptr);



void (*const statetable[MAX_STATES][MAX_EVENTS]) (void *ptr) = {
	[MENUMAIN] = {
		[INITALIZE] = state_menu_init,
		[RUN] = state_menu_run,
		[CLOSE] = state_menu_close,
		[PAUSE] = state_menu_close,
		[RESUME] = state_menu_init,
		[SDLEVENT] = state_menu_event
	},
	[WORLD_LOAD] = {
		[INITALIZE] = state_world_load,
		[RUN] = state_world_loop,
		[CLOSE] = 0,
		[PAUSE] = 0,
		[RESUME] = 0,
		[SDLEVENT] = 0
	},
	[WORLD_NEW] = {
		[INITALIZE] = state_world_new,
		[RUN] = state_world_loop,
		[CLOSE] = 0,
		[PAUSE] = 0,
		[RESUME] = 0,
		[SDLEVENT] = 0
	},
	[GAME] = {
		[INITALIZE] = state_game_init,
		[RUN] = state_game_run,
		[CLOSE] = state_game_close,
		[PAUSE] = state_game_pause,
		[RESUME] = state_game_resume,
		[SDLEVENT] = state_game_event
	},
	[INVENTORY] = {
		[INITALIZE] = state_inventory_init,
		[RUN] = state_inventory_run,
		[CLOSE] = state_inventory_close,
		[PAUSE] = 0,
		[RESUME] = 0,
		[SDLEVENT] = state_inventory_event
	}
};
