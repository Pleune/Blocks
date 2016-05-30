#include "update.h"

#include "world.h"

void
update_run(block_t b, long3_t pos, update_flags_t flags)
{
	switch(b.id)
	{
		case WATER:
		{
			if(flags & UPDATE_FLAGS_FLOW_WATER)
			{
				int waterinme = b.metadata.number;
				if(waterinme > 0)
				{ //flow down
					block_t u = world_block_get(pos.x, pos.y -1, pos.z, 0);
					if(!BLOCK_PROPERTY_SOLID(u.id) && u.id != WATER && u.id != ERR)
					{
						u.id = WATER;
						u.metadata.number = 0;
					}
					if(u.id == WATER && u.metadata.number < SIM_WATER_LEVELS)
					{
						int transfer = imin(SIM_WATER_LEVELS - u.metadata.number, waterinme);
						waterinme -= transfer;
						u.metadata.number += transfer;
						world_block_set(pos.x, pos.y -1, pos.z, u, 2, 0, 0);
					}
				}
				if(waterinme > 0)
				{ //flow down
					block_t u = world_block_get(pos.x+1, pos.y -1, pos.z, 0);
					if(!BLOCK_PROPERTY_SOLID(u.id) && u.id != WATER && u.id != ERR)
					{
						u.id = WATER;
						u.metadata.number = 0;
					}
					if(u.id == WATER && u.metadata.number < SIM_WATER_LEVELS)
					{
						int transfer = imin(SIM_WATER_LEVELS - u.metadata.number, waterinme);
						waterinme -= transfer;
						u.metadata.number += transfer;
						world_block_set(pos.x+1, pos.y -1, pos.z, u, 2, 0, 0);
					}
				}
				if(waterinme > 0)
				{ //flow down
					block_t u = world_block_get(pos.x-1, pos.y -1, pos.z, 0);
					if(!BLOCK_PROPERTY_SOLID(u.id) && u.id != WATER && u.id != ERR)
					{
						u.id = WATER;
						u.metadata.number = 0;
					}
					if(u.id == WATER && u.metadata.number < SIM_WATER_LEVELS)
					{
						int transfer = imin(SIM_WATER_LEVELS - u.metadata.number, waterinme);
						waterinme -= transfer;
						u.metadata.number += transfer;
						world_block_set(pos.x-1, pos.y -1, pos.z, u, 2, 0, 0);
					}
				}
				if(waterinme > 0)
				{ //flow down
					block_t u = world_block_get(pos.x, pos.y -1, pos.z+1, 0);
					if(!BLOCK_PROPERTY_SOLID(u.id) && u.id != WATER && u.id != ERR)
					{
						u.id = WATER;
						u.metadata.number = 0;
					}
					if(u.id == WATER && u.metadata.number < SIM_WATER_LEVELS)
					{
						int transfer = imin(SIM_WATER_LEVELS - u.metadata.number, waterinme);
						waterinme -= transfer;
						u.metadata.number += transfer;
						world_block_set(pos.x, pos.y -1, pos.z+1, u, 2, 0, 0);
					}
				}
				if(waterinme > 0)
				{ //flow down
					block_t u = world_block_get(pos.x, pos.y -1, pos.z-1, 0);
					if(!BLOCK_PROPERTY_SOLID(u.id) && u.id != WATER && u.id != ERR)
					{
						u.id = WATER;
						u.metadata.number = 0;
					}
					if(u.id == WATER && u.metadata.number < SIM_WATER_LEVELS)
					{
						int transfer = imin(SIM_WATER_LEVELS - u.metadata.number, waterinme);
						waterinme -= transfer;
						u.metadata.number += transfer;
						world_block_set(pos.x, pos.y -1, pos.z-1, u, 2, 0, 0);
					}
				}

				int delay = 2;

				if(waterinme > 0)
				{ //flow sides
					block_t u[4];
					u[0] = world_block_get(pos.x+1, pos.y, pos.z, 0);
					u[1] = world_block_get(pos.x-1, pos.y, pos.z, 0);
					u[2] = world_block_get(pos.x, pos.y, pos.z+1, 0);
					u[3] = world_block_get(pos.x, pos.y, pos.z-1, 0);

					int sum = waterinme;
					int num = 1;

					int i;
					for(i=0; i<4; i++)
					{
						if(!BLOCK_PROPERTY_SOLID(u[i].id) && u[i].id != WATER && u[i].id != ERR)
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
									world_block_set(pos.x+1, pos.y, pos.z, u[0], delay, 0, 0);
								}
							}
							if(u[1].id == WATER && u[1].metadata.number < waterinme)
							{
								if(u[1].metadata.number != avg)
								{
									u[1].metadata.number = avg;
									world_block_set(pos.x-1, pos.y, pos.z, u[1], delay, 0, 0);
								}
							}
							if(u[2].id == WATER && u[2].metadata.number < waterinme)
							{
								if(u[2].metadata.number != avg)
								{
									u[2].metadata.number = avg;
									world_block_set(pos.x, pos.y, pos.z+1, u[2], delay, 0, 0);
								}
							}
							if(u[3].id == WATER && u[3].metadata.number < waterinme)
							{
								if(u[3].metadata.number != avg)
								{
									u[3].metadata.number = avg;
									world_block_set(pos.x, pos.y, pos.z-1, u[3], delay, 0, 0);
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
					world_block_set(pos.x, pos.y, pos.z, b, 2, 0, 0);
				}
			} else {
				world_update_queue(pos.x, pos.y, pos.z, 4, UPDATE_FLAGS_FLOW_WATER);
			}

			break;
		}
		case WATER_GEN:
		{
			block_t water;
			water.id = WATER;
			water.metadata.number = SIM_WATER_LEVELS;

			world_block_set(
					pos.x, pos.y-1, pos.z,
					water,
					2, 0, 0
				);
			break;
		}
		case SAND:
		{
			if(flags & UPDATE_FLAGS_FALL)
			{
				if(!BLOCK_PROPERTY_SOLID(world_block_get(pos.x, pos.y-1, pos.z, 0).id))
				{
					world_block_set(
							pos.x, pos.y-1, pos.z,
							b,
							1, 0, 0
						);
					world_block_set_id(
							pos.x, pos.y, pos.z,
							AIR,
							1, 0, 0
						);
				}
			} else {
				world_update_queue(pos.x, pos.y, pos.z, 1, UPDATE_FLAGS_FALL);
			}
			break;
		}
		default:
			break;
	}
}
