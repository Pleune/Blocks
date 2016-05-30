#include "state.h"

#include <stdio.h>
#include <SDL2/SDL.h>

void
state_menu_init(void *ptr)
{
	printf("p to play, ESC to quit\n");
}

static uint32_t ticks = 0;
static int fpsmax = 60;
static int fpscap = 1;
void
state_menu_run(void *ptr)
{
	uint32_t newticks = SDL_GetTicks();
	uint32_t dt = ticks ? newticks - ticks : 0;
	ticks = newticks;

	if(fpscap)
	{
		uint32_t ticksdiff = SDL_GetTicks() - ticks;
		if(ticksdiff < 1000 / fpsmax)
			SDL_Delay(1000 / fpsmax - ticksdiff);
	}
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
				state_queue_push(GAME);
				break;
			case SDLK_ESCAPE:
				state_queue_pop();
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
