#include "state.h"

#include <SDL.h>

#include "world.h"
#include "worldgen.h"
#include "debug.h"

void
state_world_load(void *ptr)
{
}

void
state_world_new(void *ptr)
{
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
	world_init(spawn, &status);

	while(status != -1)
		SDL_Delay(5);

	state_queue_switch(GAME, 0);
}
