#include "block.h"
#include "world.h"
#include "custommath.h"

const blockdata_t blockinfo[(enum block_id) ERR + 1] = {
	[AIR] = {0, {0,0,0}, "Air"},
	[STONE] = {1, {0.2,0.2,0.22}, "Stone"},
	[DIRT] = {1, {0.185,0.09,0.05}, "Dirt"},
	[GRASS] = {1, {0.05,0.27,0.1}, "Grass"},
	[SAND] = {1, {0.28,0.3,0.15}, "Sand"},
	[BEDROCK] = {1, {0.1,0.1,0.1}, "Hard Stone"},
	[WATER] = {1, {0.08,0.08,0.3}, "Water"},
	[WATER_GEN] = {1, {.8,.8,.8}, "Water Generator"},
	[ERR] = {0, {1,0,0}, "Error"}
};


int
block_issolid(block_t b)
{
	if(b.id != ERR)
		return blockinfo[b.id].issolid;
	else
		return 0;
}

vec3_t
block_getcolor(blockid_t id)
{
	return blockinfo[id].color;
}

void
block_updaterun(block_t b, long3_t pos, update_flags_t flags)
{
	switch(b.id)
	{
		case WATER:
		{
			if(flags & UPDATE_FLOWWATER)
			{
				int waterinme = b.metadata.number;
				if(waterinme > 0)
				{ //flow down
					block_t u = world_getblock(pos.x, pos.y -1, pos.z, 0);
					if(!block_issolid(u) && u.id != WATER && u.id != ERR)
					{
						u.id = WATER;
						u.metadata.number = 0;
					}
					if(u.id == WATER && u.metadata.number < SIM_WATER_LEVELS)
					{
						int transfer = imin(SIM_WATER_LEVELS - u.metadata.number, waterinme);
						waterinme -= transfer;
						u.metadata.number += transfer;
						world_setblock(pos.x, pos.y -1, pos.z, u, 2, 0, 0);
					}
				}
				if(waterinme > 0)
				{ //flow down
					block_t u = world_getblock(pos.x+1, pos.y -1, pos.z, 0);
					if(!block_issolid(u) && u.id != WATER && u.id != ERR)
					{
						u.id = WATER;
						u.metadata.number = 0;
					}
					if(u.id == WATER && u.metadata.number < SIM_WATER_LEVELS)
					{
						int transfer = imin(SIM_WATER_LEVELS - u.metadata.number, waterinme);
						waterinme -= transfer;
						u.metadata.number += transfer;
						world_setblock(pos.x+1, pos.y -1, pos.z, u, 2, 0, 0);
					}
				}
				if(waterinme > 0)
				{ //flow down
					block_t u = world_getblock(pos.x-1, pos.y -1, pos.z, 0);
					if(!block_issolid(u) && u.id != WATER && u.id != ERR)
					{
						u.id = WATER;
						u.metadata.number = 0;
					}
					if(u.id == WATER && u.metadata.number < SIM_WATER_LEVELS)
					{
						int transfer = imin(SIM_WATER_LEVELS - u.metadata.number, waterinme);
						waterinme -= transfer;
						u.metadata.number += transfer;
						world_setblock(pos.x-1, pos.y -1, pos.z, u, 2, 0, 0);
					}
				}
				if(waterinme > 0)
				{ //flow down
					block_t u = world_getblock(pos.x, pos.y -1, pos.z+1, 0);
					if(!block_issolid(u) && u.id != WATER && u.id != ERR)
					{
						u.id = WATER;
						u.metadata.number = 0;
					}
					if(u.id == WATER && u.metadata.number < SIM_WATER_LEVELS)
					{
						int transfer = imin(SIM_WATER_LEVELS - u.metadata.number, waterinme);
						waterinme -= transfer;
						u.metadata.number += transfer;
						world_setblock(pos.x, pos.y -1, pos.z+1, u, 2, 0, 0);
					}
				}
				if(waterinme > 0)
				{ //flow down
					block_t u = world_getblock(pos.x, pos.y -1, pos.z-1, 0);
					if(!block_issolid(u) && u.id != WATER && u.id != ERR)
					{
						u.id = WATER;
						u.metadata.number = 0;
					}
					if(u.id == WATER && u.metadata.number < SIM_WATER_LEVELS)
					{
						int transfer = imin(SIM_WATER_LEVELS - u.metadata.number, waterinme);
						waterinme -= transfer;
						u.metadata.number += transfer;
						world_setblock(pos.x, pos.y -1, pos.z-1, u, 2, 0, 0);
					}
				}

				int delay = 2;

				if(waterinme > 0)
				{ //flow sides
					block_t u[4];
					u[0] = world_getblock(pos.x+1, pos.y, pos.z, 0);
					u[1] = world_getblock(pos.x-1, pos.y, pos.z, 0);
					u[2] = world_getblock(pos.x, pos.y, pos.z+1, 0);
					u[3] = world_getblock(pos.x, pos.y, pos.z-1, 0);

					int sum = waterinme;
					int num = 1;

					int i;
					for(i=0; i<4; i++)
					{
						if(!block_issolid(u[i]) && u[i].id != WATER && u[i].id != ERR)
						{
							u[i].id = WATER;
							u[i].metadata.number = 0;
						}
						if(u[i].id == WATER && u[i].metadata.number < waterinme)
						{
							num++;
							sum += u[i].metadata.number;
						}
					}

					if(num > 1)
					{
						int avg = sum/num;
						int rem = sum % num;

						delay += SIM_WATER_DELAY;

						if(avg > 0 && avg != waterinme)
						{
							if(u[0].id == WATER && u[0].metadata.number < waterinme)
							{
								if(u[0].metadata.number != avg)
								{
									u[0].metadata.number = avg;
									world_setblock(pos.x+1, pos.y, pos.z, u[0], delay, 0, 0);
								}
							}
							if(u[1].id == WATER && u[1].metadata.number < waterinme)
							{
								if(u[1].metadata.number != avg)
								{
									u[1].metadata.number = avg;
									world_setblock(pos.x-1, pos.y, pos.z, u[1], delay, 0, 0);
								}
							}
							if(u[2].id == WATER && u[2].metadata.number < waterinme)
							{
								if(u[2].metadata.number != avg)
								{
									u[2].metadata.number = avg;
									world_setblock(pos.x, pos.y, pos.z+1, u[2], delay, 0, 0);
								}
							}
							if(u[3].id == WATER && u[3].metadata.number < waterinme)
							{
								if(u[3].metadata.number != avg)
								{
									u[3].metadata.number = avg;
									world_setblock(pos.x, pos.y, pos.z-1, u[3], delay, 0, 0);
								}
							}

							waterinme = avg + rem;
						}
					}
				}
				if(waterinme > 0)
				{ //flow up
				}

				if(waterinme < 1)
					b.id = AIR;

				if(waterinme != b.metadata.number || b.id != WATER)
				{
					b.metadata.number = waterinme;
					world_setblock(pos.x, pos.y, pos.z, b, 2, 0, 0);
				}
			} else {
				world_updatequeue(pos.x, pos.y, pos.z, 4, UPDATE_FLOWWATER);
			}

			break;
		}
		case WATER_GEN:
		{
			block_t water;
			water.id = WATER;
			water.metadata.number = SIM_WATER_LEVELS;

			world_setblock(
					pos.x, pos.y-1, pos.z,
					water,
					2, 0, 0
				);
			break;
		}
		case SAND:
		{
			if(flags & UPDATE_FALL)
			{
				if(!block_issolid(world_getblock(pos.x, pos.y-1, pos.z, 0)))
				{
					world_setblock(
							pos.x, pos.y-1, pos.z,
							b,
							1, 0, 0
						);
					world_setblockid(
							pos.x, pos.y, pos.z,
							AIR,
							1, 0, 0
						);
				}
			} else {
				world_updatequeue(pos.x, pos.y, pos.z, 1, UPDATE_FALL);
			}
			break;
		}
		default:
			break;
	}
}
