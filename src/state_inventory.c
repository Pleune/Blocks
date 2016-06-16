#include "state.h"

#include <SDL.h>
#include <GL/glew.h>

#include "state_game.h"
#include "textbox.h"
#include "debug.h"

static uint32_t ticks = 0;
static uint32_t fpsmax = 60;
static int fpscap = 1;

static textbox_t *textbox_title;
static textbox_t *textbox_play;

void
state_inventory_init(void *ptr)
{
	if(!state_is_initalized(GAME))
	{
		error("Inventory loaded before game!");
		state_queue_pop();
	}

	textbox_title = textbox_create(
		80, 10, 600, 100,
		"INVENTORY",
		TEXTBOX_COLOR_WHITE,
		TEXTBOX_FONT_ROBOTO_BOLD,
		TEXTBOX_FONT_SIZE_HUGE,
		0);

	textbox_play = textbox_create(
		80, 80, 300, 100,
		"Testing...!",
		TEXTBOX_COLOR_BLUE,
		TEXTBOX_FONT_ROBOTO_REGULAR,
		TEXTBOX_FONT_SIZE_MEDIUM,
		0);
}

void
state_inventory_run(void *ptr)
{
	uint32_t newticks = SDL_GetTicks();
	//	uint32_t dt = ticks ? newticks - ticks : 0;
	ticks = newticks;

	glClear(GL_COLOR_BUFFER_BIT);

	state_game_update();
	state_game_render();

	textbox_render(textbox_title);
	textbox_render(textbox_play);

	state_window_swap();

	if(fpscap)
	{
		uint32_t ticksdiff = SDL_GetTicks() - ticks;
		if(ticksdiff < 1000 / fpsmax)
			SDL_Delay(1000 / fpsmax - ticksdiff);
	}
}

void
state_inventory_close(void *ptr)
{
	textbox_destroy(textbox_title);
	textbox_destroy(textbox_play);
}

void
state_inventory_event(void *ptr)
{
	SDL_Event *e = (SDL_Event *)ptr;
	if(e->type == SDL_KEYDOWN)
	{
		switch(e->key.keysym.sym)
		{
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
