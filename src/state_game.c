#include "state_game.h"

#include <stdint.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "state.h"

uint32_t ticks = 0;

int fpscap = 1;
const int fpsmax = 60;


void
state_game_init()
{

}

void
state_game_run()
{
	ticks = SDL_GetTicks();

	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		//handle input
		if(e.type == SDL_KEYDOWN)
		{
			switch(e.key.keysym.sym)
			{
				case SDLK_ESCAPE:
					state_changeto(CLOSING);
				break;
			}
		}
	}

	glClear(GL_COLOR_BUFFER_BIT);
	swapWindow();

	if(fpscap)
	{
		uint32_t ticksdiff = SDL_GetTicks() - ticks;
		if(ticksdiff < 1000 / fpsmax)
			SDL_Delay(1000 / fpsmax - ticksdiff);
	}
}

void
state_game_close()
{

}
