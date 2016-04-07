#include "state.h"

#include <stdio.h>
#include <SDL2/SDL.h>

void
state_menu_init()
{
	printf("p to play, ESC to quit\n");
}

void
state_menu_run()
{
	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		if(e.type == SDL_KEYDOWN)
		{
			switch(e.key.keysym.sym)
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
}

void
state_menu_close()
{

}
