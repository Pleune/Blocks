#include "state.h"

void (*const statetable[MAX_STATES][MAX_EVENTS]) (void *ptr) = {
	[MENUMAIN] = {
		[INITALIZE] = state_menu_init,
		[RUN] = state_menu_run,
		[CLOSE] = state_menu_close,
		[PAUSE] = state_menu_close,
		[RESUME] = state_menu_init,
		[SDLEVENT] = state_menu_event
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
