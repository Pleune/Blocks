#include "state.h"

#include <stdio.h>
#include <SDL.h>

#include "textbox.h"

static uint32_t ticks = 0;
static uint32_t fpsmax = 60;
static int fpscap = 1;

static textbox_t *text;

void
state_menu_init(void *ptr)
{
	printf("p to play, ESC to quit\n");
	glClearColor(0, 0, 0, 1);

	text = textbox_create(10,10,100,100,"A B C D E F G this is a TeSt!!!! !@^*$%&!@",STANDARD);
}

void
state_menu_run(void *ptr)
{
	uint32_t newticks = SDL_GetTicks();
	//	uint32_t dt = ticks ? newticks - ticks : 0;
	ticks = newticks;

	glClear(GL_COLOR_BUFFER_BIT);

	textbox_render(text);

	state_window_swap();

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
	else if(e->type == SDL_WINDOWEVENT)
	{
		if(e->window.event == SDL_WINDOWEVENT_RESIZED)
		{
		}
	}
}

void
state_menu_close(void *ptr)
{
	textbox_destroy(text);
}
