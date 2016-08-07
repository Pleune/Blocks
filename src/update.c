#include "update.h"

#include "world.h"
#include "stack.h"
#include "save.h"

update_stack_t *
update_stack_create()
{
	struct update_stack *ret = malloc(sizeof(struct update_stack));
	ret->mutex = SDL_CreateMutex();
	ret->queue = 0;
	ret->misses = 0;

	return ret;
}

void
update_stack_destroy(update_stack_t *stack)
{
	update_stack_clear(stack);

	SDL_DestroyMutex(stack->mutex);
	free(stack);
}

void
update_stack_clear(update_stack_t *stack)
{
	SDL_LockMutex(stack->mutex);

	struct update_node *updates = stack->queue;
	while(updates)
	{
		struct update_node *next = updates->next;
		free(updates);
		updates = next;
	}

	stack->queue = 0;

	SDL_UnlockMutex(stack->mutex);
}

void
update_queue(update_stack_t *stack, long x, long y, long z, int time, update_flags_t flags)
{
	struct update_node *new = malloc(sizeof(struct update_node));
	new->next = 0;

	new->pos.x = x;
	new->pos.y = y;
	new->pos.z = z;

	new->time = time;
	new->flags = flags;

	SDL_LockMutex(stack->mutex);

	if(stack->queue)
	{
		struct update_node *top = stack->queue;
		while(top->next)
		{
			if(new->pos.x == top->pos.x && new->pos.y == top->pos.y && new->pos.z == top->pos.z)
			{
				if(top->time > time)
					top->time = time;
				top->flags |= flags;
				free(new);

				SDL_UnlockMutex(stack->mutex);
				return;
			}
			top = top->next;
		}
		top->next = new;
	} else {
		stack->queue = new;
	}

	SDL_UnlockMutex(stack->mutex);
}

int
update_run(update_stack_t *stack)
{
	stack->misses++;

	if(SDL_TryLockMutex(stack->mutex) != 0)
	{
		stack->misses++;
		return BLOCKS_ERROR;
	}

	int num = 0;

	while(stack->misses > 0)
	{
		if(stack->queue)
		{
			struct update_node *node = stack->queue;
			struct update_node *prev = 0;

			while(node)
			{
				struct update_node *next = node->next;
				if(node->time == 0)
				{
					if(stack->queue == node)
						stack->queue = node->next;

					if(prev != 0)
						prev->next = node->next;

					update_run_single(node);
					num++;

					free(node);
				} else {
					node->time--;
					prev = node;
				}
				node = next;
			}
		}

		stack->misses--;
	}
	SDL_UnlockMutex(stack->mutex);

	return num;
}

void
update_run_single(const struct update_node *update)
{
	long3_t pos = update->pos;
	update_flags_t flags = update->flags;

	block_t b = world_block_get(pos.x, pos.y, pos.z, 0);
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
				{
					b.id = AIR;
					b.metadata.number = 0;
				}

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

void
update_fail_once(update_stack_t *stack)
{
	stack->misses++;
}

size_t
update_dump(update_stack_t *stack, unsigned char **data)
{
	SDL_LockMutex(stack->mutex);

	//TODO: constants
	stack_t *data_stack = stack_create(1, 100, 2.0);

	unsigned char tmp[8];

	struct update_node *this = stack->queue;
	while(this)
	{
		int3_t pos = world_get_internalpos_of_worldpos(this->pos.x, this->pos.y, this->pos.z);

		save_write_uint16(tmp, this->flags);
		stack_push_mult(data_stack, tmp, 2);
		save_write_uint16(tmp, pos.x);
		stack_push_mult(data_stack, tmp, 2);
		save_write_uint16(tmp, pos.y);
		stack_push_mult(data_stack, tmp, 2);
		save_write_uint16(tmp, pos.z);
		stack_push_mult(data_stack, tmp, 2);
		save_write_uint16(tmp, this->time);
		stack_push_mult(data_stack, tmp, 2);

		this = this->next;
	}

	SDL_UnlockMutex(stack->mutex);

	size_t stack_size = stack_objects_get_num(data_stack);
	if(stack_size)
	{
		stack_trim(data_stack);
		*data = stack_transform_dataptr(data_stack);
	} else {
		stack_destroy(data_stack);
	}

	return stack_size;
}

void
update_read(update_stack_t *stack, const long3_t *cpos, const unsigned char *data, size_t size)
{
	size_t count = 0;
	for(count=0; count<size; count+=10)
	{
		struct update_node update;
		update.flags = save_read_uint16(data);
		data += 2;
		update.pos.x = save_read_uint16(data);
		data += 2;
		update.pos.y = save_read_uint16(data);
		data += 2;
		update.pos.z = save_read_uint16(data);
		data += 2;
		update.time = save_read_uint16(data);
		data += 2;

		update.pos = world_get_worldpos_of_internalpos(cpos, update.pos.x, update.pos.y, update.pos.z);

		update_queue(stack, update.pos.x, update.pos.y, update.pos.z, update.time, update.flags);
	}
}
