#include "state.h"

#include <stdio.h>
#include <SDL.h>
#include <GL/glew.h>

#include "textbox.h"

static uint32_t ticks = 0;
static uint32_t fpsmax = 60;
static int fpscap = 1;

static textbox_t *textbox_title;
static textbox_t *textbox_play;

void
state_menu_init(void *ptr)
{
	printf("p to play, ESC to quit\n");
	glClearColor(0, 0, 0, 1);

	textbox_title = textbox_create(
		10, 10, 300, 100,
		"BLOCKS",
		TEXTBOX_COLOR_GREEN,
		TEXTBOX_FONT_ROBOTO_BOLDITALIC,
		TEXTBOX_FONT_SIZE_HUGE,
		TEXTBOX_FLAG_CENTER_H);

	textbox_play = textbox_create(
		80, 80, 300, 100,
		"Press 'P' to play!",
		TEXTBOX_COLOR_BLUE,
		TEXTBOX_FONT_ROBOTO_REGULAR,
		TEXTBOX_FONT_SIZE_MEDIUM,
		0);
}

void
state_menu_run(void *ptr)
{
	uint32_t newticks = SDL_GetTicks();
	//	uint32_t dt = ticks ? newticks - ticks : 0;
	ticks = newticks;

	glClear(GL_COLOR_BUFFER_BIT);

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
	textbox_destroy(textbox_title);
	textbox_destroy(textbox_play);
}
