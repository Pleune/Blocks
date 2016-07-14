#include "state.h"

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <GL/glew.h>

#include "interface.h"

static uint32_t ticks = 0;
static uint32_t fpsmax = 60;
static int fpscap = 1;

static textbox_t *textbox_play;
static textbox_t *textbox_load;
static textbox_t *textbox_quit;
static interface_t *ui;
static int32_t ui_load;
static int32_t ui_play;
static int32_t ui_quit;

void
state_menu_init(void *ptr)
{
	glClearColor(0, 0, 0, 1);

	textbox_t *textbox_title = textbox_create(
		50, 10, 300, 100,
		"BLOCKS",
		TEXTBOX_COLOR_GREEN,
		TEXTBOX_FONT_ROBOTO_BOLDITALIC,
		TEXTBOX_FONT_SIZE_HUGE,
		0);

	textbox_play = textbox_create(
		50, 100, 120, 32,
		"Play!",
		TEXTBOX_COLOR_BLUE,
		TEXTBOX_FONT_ROBOTO_REGULAR,
		TEXTBOX_FONT_SIZE_MEDIUM,
		TEXTBOX_FLAG_CENTER_V);

	textbox_load = textbox_create(
		50, 130, 120, 32,
		"Load",
		TEXTBOX_COLOR_BLUE,
		TEXTBOX_FONT_ROBOTO_REGULAR,
		TEXTBOX_FONT_SIZE_MEDIUM,
		TEXTBOX_FLAG_CENTER_V);

	textbox_quit = textbox_create(
		50, 190, 120, 32,
		"Quit",
		TEXTBOX_COLOR_BLUE,
		TEXTBOX_FONT_ROBOTO_REGULAR,
		TEXTBOX_FONT_SIZE_MEDIUM,
		TEXTBOX_FLAG_CENTER_V);

	ui = interface_create(0);
	interface_attach_textbox(ui, textbox_title);
	ui_play = interface_attach_textbox(ui, textbox_play);
	ui_load = interface_attach_textbox(ui, textbox_load);
	ui_quit = interface_attach_textbox(ui, textbox_quit);
}

void
state_menu_run(void *ptr)
{
	uint32_t newticks = SDL_GetTicks();
	//	uint32_t dt = ticks ? newticks - ticks : 0;
	ticks = newticks;

	glClear(GL_COLOR_BUFFER_BIT);

	interface_render(ui);

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
	interface_event_detect(ui, e);

	if(e->type == INTERFACE_MOUSECLICKEVENT)
	{
		if(e->user.code == ui_play)
			state_queue_push(WORLD_NEW, 0);
		if(e->user.code == ui_load)
			state_queue_push(WORLD_LOAD, 0);
		if(e->user.code == ui_quit)
			state_queue_pop();
	}
	if(e->type == INTERFACE_MOUSEOVEREVENT)
	{
		if(e->user.code == ui_play)
			textbox_set_color(textbox_play, TEXTBOX_COLOR_WHITE);
		if(e->user.code == ui_load)
			textbox_set_color(textbox_load, TEXTBOX_COLOR_WHITE);
		if(e->user.code == ui_quit)
			textbox_set_color(textbox_quit, TEXTBOX_COLOR_WHITE);
	}
	if(e->type == INTERFACE_MOUSEOVEREXITEVENT)
	{
		if(e->user.code == ui_play)
			textbox_set_color(textbox_play, TEXTBOX_COLOR_BLUE);
		if(e->user.code == ui_load)
			textbox_set_color(textbox_load, TEXTBOX_COLOR_BLUE);
		if(e->user.code == ui_quit)
			textbox_set_color(textbox_quit, TEXTBOX_COLOR_BLUE);
	}
}

void
state_menu_close(void *ptr)
{
	interface_destroy(ui);
}
