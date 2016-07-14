#include "state.h"

#include <SDL.h>
#include <stdio.h>

#include "world.h"
#include "worldgen.h"
#include "textbox.h"
#include "debug.h"

static textbox_t *textbox_a;
static textbox_t *textbox_b;

static void
init_graphical()
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
	state_window_swap();
}

static void
loop(int status)
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
	init_graphical();

	world_set_seed(0);
	vec3_t spawn = {0, 0, 0};

	spawn.y = worldgen_get_height_of_pos(0, 0, 0)+1.1;
	/*
	int spawntries = 0;
	while((spawn.y < 0 || spawn.y > 70) && spawntries < 500)
	{
		spawntries++;
		spawn.x = (double)(rand()%10000) - 5000;
		spawn.z = (double)(rand()%10000) - 5000;
		spawn.y = worldgen_get_height_of_pos(0, spawn.x, spawn.z)+1.1;
		info("spawn retry %i x: %f z: %f h: %f", spawntries, spawn.x, spawn.z, spawn.y);
	}
	*/
	spawn.x += .5;
	spawn.z += .5;
	if(spawn.y < 0)
		spawn.y = 0.1;
	info("h: %f\n", spawn.y);

	volatile int status = 0;
	if (world_init(spawn) == -1)
	{
		state_queue_fail();
		return;
	}

	world_load(state_prefpath_get(), "savefile", &status);

	while (status != -1)
		loop(status);

	state_queue_switch(GAME, 0);
}

void
state_world_new(void *ptr)
{
	init_graphical();

	world_set_seed(0);
	vec3_t spawn = {0, 0, 0};

	spawn.y = worldgen_get_height_of_pos(0, 0, 0)+1.1;
	/*
	int spawntries = 0;
	while((spawn.y < 0 || spawn.y > 70) && spawntries < 500)
	{
		spawntries++;
		spawn.x = (double)(rand()%10000) - 5000;
		spawn.z = (double)(rand()%10000) - 5000;
		spawn.y = worldgen_get_height_of_pos(0, spawn.x, spawn.z)+1.1;
		info("spawn retry %i x: %f z: %f h: %f", spawntries, spawn.x, spawn.z, spawn.y);
	}
	*/
	spawn.x += .5;
	spawn.z += .5;
	if(spawn.y < 0)
		spawn.y = 0.1;
	info("h: %f\n", spawn.y);

	volatile int status = 0;
	if (world_init(spawn) == -1)
	{
		state_queue_fail();
		return;
	}

	world_generate(&status);

	while (status != -1)
		loop(status);

	state_queue_switch(GAME, 0);
}
