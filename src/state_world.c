#include "state.h"

#include <SDL.h>
#include <stdio.h>

#include "world.h"
#include "worldgen.h"
#include "textbox.h"
#include "debug.h"

static textbox_t *textbox_a;
static textbox_t *textbox_b;
static volatile int status;

static void
init()
{
	glClearColor(0, 0, 0, 1);

	int windoww, windowh;
	state_window_get_size(&windoww, &windowh);

	textbox_a = textbox_create(
		10, 10, 300, 40,
		"0% complete",
		TEXTBOX_COLOR_WHITE,
		TEXTBOX_FONT_ROBOTO_BOLD,
		TEXTBOX_FONT_SIZE_LARGE,
		0);

	textbox_b = textbox_create(
		10, 40, 500, 40,
		"0 chunks generated",
		TEXTBOX_COLOR_WHITE,
		TEXTBOX_FONT_ROBOTO_BOLD,
		TEXTBOX_FONT_SIZE_LARGE,
		0);

	glClear(GL_COLOR_BUFFER_BIT);
	textbox_render(textbox_a);
	textbox_render(textbox_b);
	info("init");
	state_window_swap();

	status = 0;
}

void
state_world_loop(void *ptr)
{
	static uint32_t ticks;
	uint32_t newticks = SDL_GetTicks();
	//	uint32_t dt = ticks ? newticks - ticks : 0;
	ticks = newticks;

	int percent = 100*status/(WORLD_CHUNKS_PER_EDGE*WORLD_CHUNKS_PER_EDGE*WORLD_CHUNKS_PER_EDGE);

	char txt[128];
	snprintf(txt, sizeof(txt), "%i%% complete", percent);
	textbox_set_txt(textbox_a, txt);
	snprintf(txt, sizeof(txt), "%i chunks generated", status);
	textbox_set_txt(textbox_b, txt);

	glClear(GL_COLOR_BUFFER_BIT);
	textbox_render(textbox_a);
	textbox_render(textbox_b);
	state_window_swap();

	if(status == -1)
		state_queue_switch(GAME, 0);

	//TODO: option
	if(1)
	{
		uint32_t ticksdiff = SDL_GetTicks() - ticks;
		if(ticksdiff < 1000 / 60)
			SDL_Delay(1000 / 60 - ticksdiff);
	}
}

void
state_world_load(void *ptr)
{
	init();

	if (world_init_load("savename", &status) == -1)
	{
		state_queue_pop();
		return;
	}
}

void
state_world_new(void *ptr)
{
	init();

	if (world_init_new(&status, "savename") == -1)
	{
		state_queue_pop();
		return;
	}
}

void
state_world_cleanup(void* ptr)
{
	textbox_destroy(textbox_a);
	textbox_destroy(textbox_b);
	info("clear");
}
