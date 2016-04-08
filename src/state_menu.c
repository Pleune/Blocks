#include "state.h"

#include <stdio.h>
#include <SDL2/SDL.h>

void
state_menu_init(void *ptr)
{
	printf("p to play, ESC to quit\n");
}

void
state_menu_run(void *ptr)
{
}

void
state_menu_event(void *ptr)
{
	SDL_Event *e = (SDL_Event *)ptr;
	if(e->type == SDL_KEYDOWN)
	{
		switch(e->key.keysym.sym)
		{
			case SDLK_p:
				state_queuepush(GAME);
				break;
			case SDLK_ESCAPE:
				state_queuepop();
				break;
			default:
				break;
		}
	}
}

void
state_menu_close(void *ptr)
{

}
