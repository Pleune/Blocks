#include "state.h"

void (*const statetable[MAX_STATES][MAX_EVENTS]) (void *ptr) = {
	[MENUMAIN] = {
		[INITALIZE] = state_menu_init,
		[RUN] = state_menu_run,
		[CLOSE] = state_menu_close,
		[PAUSE] = 0,
		[RESUME] = 0,
		[SDLEVENT] = state_menu_event
	},
	[GAME] = {
		[INITALIZE] = state_game_init,
		[RUN] = state_game_run,
		[CLOSE] = state_game_close,
		[PAUSE] = 0,
		[RESUME] = 0,
		[SDLEVENT] = state_game_event
	}
};
