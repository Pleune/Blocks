#include "block.h"
#include "world.h"

const blockdata_t blockinfo[(enum block_id) ERR] = {
	[AIR] = {0, {0,0,0}, "Air"},
	[STONE] = {1, {0.2,0.2,0.22}, "Stone"},
	[DIRT] = {1, {0.185,0.09,0.05}, "Dirt"},
	[GRASS] = {1, {0.05,0.27,0.1}, "Grass"},
	[BEDROCK] = {1, {0.1,0.1,0.1}, "Hard Stone"},
	[WATER] = {1, {0.08,0.08,0.3}, "Water"}
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
block_updaterun(blockid_t id, long3_t pos, update_flags_t flags)
{
	switch(id)
	{
		case WATER:
		{
			if(flags & UPDATE_FLOWWATER)
			{
				if(!block_issolid(world_getblock(pos.x+1, pos.y, pos.z, 0)))
					world_setblock(
							pos.x+1, pos.y, pos.z,
							world_getblock(pos.x, pos.y, pos.z, 0),
							1, 0, 0
						);
				//if(!block_issolid(world_getblock(pos.x, pos.y+1, pos.z, 0)))
				//	world_setblock(
				//			pos.x+1, pos.y, pos.z,
				//			world_getblock(pos.x, pos.y, pos.z, 0),
				//			1, 0, 0
				//		);
				if(!block_issolid(world_getblock(pos.x, pos.y, pos.z+1, 0)))
					world_setblock(
							pos.x, pos.y, pos.z+1,
							world_getblock(pos.x, pos.y, pos.z, 0),
							1, 0, 0
						);
				if(!block_issolid(world_getblock(pos.x-1, pos.y, pos.z, 0)))
					world_setblock(
							pos.x-1, pos.y, pos.z,
							world_getblock(pos.x, pos.y, pos.z, 0),
							1, 0, 0
						);
				if(!block_issolid(world_getblock(pos.x, pos.y-1, pos.z, 0)))
					world_setblock(
							pos.x, pos.y-1, pos.z,
							world_getblock(pos.x, pos.y, pos.z, 0),
							1, 0, 0
						);
				if(!block_issolid(world_getblock(pos.x, pos.y, pos.z-1, 0)))
					world_setblock(
							pos.x, pos.y, pos.z-1,
							world_getblock(pos.x, pos.y, pos.z, 0),
							1, 0, 0
						);
			} else {
				world_updatequeue(pos.x, pos.y, pos.z, 1, UPDATE_FLOWWATER);
			}
			break;
		}
		default:
			break;
	}
}
