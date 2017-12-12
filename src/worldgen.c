#include "worldgen.h"

#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include "world.h"
#include "custommath.h"
#include "defines.h"
#include "modulo.h"
#include "noise.h"

#define DIAMONDSQUARESIZE (int) CAT(0x1p, WORLDGEN_DIAMONDSQUARE_LEVELS)

struct worldgen_s {
	long3_t lastchunkblockpos;
	long3_t lastdiasquareblockpos;
	double heightmap[(CHUNKSIZE+1)*(CHUNKSIZE+1)];
	double metaheightmap[(DIAMONDSQUARESIZE+1)*(DIAMONDSQUARESIZE+1)];
};

worldgen_t defaultcontext = {
	{LONG_MAX,LONG_MAX,LONG_MAX},
	{LONG_MAX,LONG_MAX,LONG_MAX}
};

/**
 * flattens the world out around y = 0
 */
static void
bias(double *data, long3_t wpos)
{
	int x, z;
	for(x=0; x<CHUNKSIZE+1; ++x)
    {
        long x_ = x + wpos.x;
        if(x_ < 0) x_ = -x_;
        x_ -= 8;
        if(x_ < 0) x_ = 0;
        if(x_ > 100) x_ = 100;

        float xbias = x_ / 100.0f;

        xbias *= xbias;

        for(z=0; z<CHUNKSIZE+1; z++)
        {
            int index = x + z*(CHUNKSIZE+1);
            double h = data[index];

            data[index] = (h*h*h*h*h)/(h*h*h*h+300*h*h) * xbias;
        }
    }
}

static double
weight(double d)
{
	double val = d / (WORLDGEN_RANGE * DIAMONDSQUARESIZE);
	val = (val + 1.0)/2.0;
	if(val < 0)
		return 0;
	else if(val > 1)
		return 1;
	else
		return val;
}

/**
 * applys the diamond square algo.
 */
static void
pound(double *data, size_t size, long3_t pos, uint32_t seed, int scale, int levels)
{
	int step;
	for(step = levels-1; step>=0; step--)
	{
		int x, z;
		int d_ = step == 0 ? 1 : pow(2, step);
		int d = d_ * 2;

		//randomness
		int r = d_ * scale*.5 * WORLDGEN_BUMPYNESS;

		//square
		for(x=d_; x<size; x+=d)
		{
			for(z=d_; z<size; z+=d)
			{
				data[x + z*size] = (
						data[(x-d_) + (z-d_)*size] +
						data[(x-d_) + (z+d_)*size] +
						data[(x+d_) + (z-d_)*size] +
						data[(x+d_) + (z+d_)*size]
						) / 4.0;
				data[x + z*size] += (double)((noise2D(pos.x + x*scale, pos.z + z*scale, seed)%100)/100.0 - weight(data[x + z*size]))*r;
			}
		}

		//diamond
		for(z=0; z<size; z+=d)
		{
			for(x=d_; x<size; x+=d)
			{
				/*
				 * If on edge of chunk:
				 * only use values on the edge, so that the values are the same as the neibooring chunks' edges
				 */
				int notedge = (z-d_>=0) && (z+d_ <size);

				data[x + z*size] = (data[x-d_ + z*size] + data[x+d_ + z*size]);
				data[z + x*size] = (data[z + (x-d_)*size] + data[z + (x+d_)*size]);
				if(notedge)
				{
					data[x + z*size] += data[x + (z-d_)*size];
					data[z + x*size] += data[z-d_ + x*size];
					data[x + z*size] += data[x + (z+d_)*size];
					data[z + x*size] += data[z+d_ + x*size];
				}

				data[x + z*size] /= notedge ? 4.0 : 2.0;
				data[x + z*size] += (double)((noise2D(pos.x + x*scale, pos.z + z*scale, seed)%100)/100.0 - weight(data[x + z*size]))*r;
				data[z + x*size] /= notedge ? 4.0 : 2.0;
				data[z + x*size] += (double)((noise2D(pos.x + z*scale, pos.z + x*scale, seed)%100)/100.0 - weight(data[z + x*size]))*r;
			}
		}
	}
}

static long3_t
setheightmapfromcpos(worldgen_t *context, long3_t cpos)
{
	uint32_t seed = world_get_seed();

	double *heightmap = context->heightmap;
	double *metaheightmap = context->metaheightmap;

	long3_t newchunkblockpos;
	newchunkblockpos.x = cpos.x * CHUNKSIZE;
	newchunkblockpos.y = cpos.y * CHUNKSIZE;
	newchunkblockpos.z = cpos.z * CHUNKSIZE;

	if(context->lastchunkblockpos.x != newchunkblockpos.x
		|| context->lastchunkblockpos.z != newchunkblockpos.z
		|| (context->lastchunkblockpos.x == LONG_MAX || context->lastchunkblockpos.z == LONG_MAX))
	{
		context->lastchunkblockpos = newchunkblockpos;
		long3_t newdiasquareblockpos = {
			floor((double)cpos.x / (double)DIAMONDSQUARESIZE) * DIAMONDSQUARESIZE * CHUNKSIZE,
			floor((double)cpos.y / (double)DIAMONDSQUARESIZE) * DIAMONDSQUARESIZE * CHUNKSIZE,
			floor((double)cpos.z / (double)DIAMONDSQUARESIZE) * DIAMONDSQUARESIZE * CHUNKSIZE
		};

		if(context->lastdiasquareblockpos.x != newdiasquareblockpos.x || context->lastdiasquareblockpos.z != newdiasquareblockpos.z || (context->lastdiasquareblockpos.x == LONG_MAX || context->lastdiasquareblockpos.z == LONG_MAX))
		{
			context->lastdiasquareblockpos = newdiasquareblockpos;

			metaheightmap[0] =
				((noise2D(
					newdiasquareblockpos.x,
					newdiasquareblockpos.z,
					seed
					)%100)/100.0 - .5) * (DIAMONDSQUARESIZE*WORLDGEN_RANGE);
			metaheightmap[DIAMONDSQUARESIZE] =
				((noise2D(
					newdiasquareblockpos.x + DIAMONDSQUARESIZE*CHUNKSIZE,
					newdiasquareblockpos.z,
					seed
					)%100)/100.0 - .5) * (DIAMONDSQUARESIZE*WORLDGEN_RANGE);
			metaheightmap[(DIAMONDSQUARESIZE+1)*DIAMONDSQUARESIZE] =
				((noise2D(
					newdiasquareblockpos.x,
					newdiasquareblockpos.z + DIAMONDSQUARESIZE*CHUNKSIZE,
					seed
					)%100)/100.0 - .5) * (DIAMONDSQUARESIZE*WORLDGEN_RANGE);
			metaheightmap[DIAMONDSQUARESIZE + (DIAMONDSQUARESIZE+1)*DIAMONDSQUARESIZE] =
				((noise2D(
					newdiasquareblockpos.x + DIAMONDSQUARESIZE*CHUNKSIZE,
					newdiasquareblockpos.z + DIAMONDSQUARESIZE*CHUNKSIZE,
					seed
					)%100)/100.0 - .5) * (DIAMONDSQUARESIZE*WORLDGEN_RANGE);

			pound(metaheightmap, DIAMONDSQUARESIZE+1, newdiasquareblockpos, seed, CHUNKSIZE, WORLDGEN_DIAMONDSQUARE_LEVELS);
		}

		long3_t inewdiasquareblockpos = {
			MODULO(cpos.x, DIAMONDSQUARESIZE),
			MODULO(cpos.y, DIAMONDSQUARESIZE),
			MODULO(cpos.z, DIAMONDSQUARESIZE)
		};

		heightmap[0					] = metaheightmap[inewdiasquareblockpos.x + 		inewdiasquareblockpos.z*(DIAMONDSQUARESIZE+1)];
		heightmap[CHUNKSIZE				] = metaheightmap[(inewdiasquareblockpos.x+1) + 	inewdiasquareblockpos.z*(DIAMONDSQUARESIZE+1)];
		heightmap[		CHUNKSIZE*(CHUNKSIZE+1)	] = metaheightmap[inewdiasquareblockpos.x + 		(inewdiasquareblockpos.z+1)*(DIAMONDSQUARESIZE+1)];
		heightmap[CHUNKSIZE + 	CHUNKSIZE*(CHUNKSIZE+1)	] = metaheightmap[(inewdiasquareblockpos.x+1) + 	(inewdiasquareblockpos.z+1)*(DIAMONDSQUARESIZE+1)];

		pound(heightmap, CHUNKSIZE+1, newchunkblockpos, seed, 1, CHUNK_LEVELS);
		bias(heightmap, newchunkblockpos);
	}
	return newchunkblockpos;
}

static double
getheightval(worldgen_t *context, long x, long z)
{
	double *heightmap = context->heightmap;
	return (
			heightmap[x+1 + z*(CHUNKSIZE+1)] +
			heightmap[x + z*(CHUNKSIZE+1)] +
			heightmap[x+1 + (z+1)*(CHUNKSIZE+1)] +
			heightmap[x + (z+1)*(CHUNKSIZE+1)
		]) / 4.0;
}

worldgen_t *
worldgen_context_create()
{
	worldgen_t *ret = malloc(sizeof(worldgen_t));

	ret->lastchunkblockpos.x = LONG_MAX;
	ret->lastchunkblockpos.y = LONG_MAX;
	ret->lastchunkblockpos.z = LONG_MAX;
	ret->lastdiasquareblockpos.x = LONG_MAX;
	ret->lastdiasquareblockpos.y = LONG_MAX;
	ret->lastdiasquareblockpos.z = LONG_MAX;

	return ret;
}

void
worldgen_context_destroy(worldgen_t *context)
{
	free(context);
}

void
worldgen_genchunk(worldgen_t *context, chunk_t *chunk, long3_t *cpos)
{
	if(context == 0)
		context = &defaultcontext;

	chunk_lock(chunk);
	chunk_recenter(chunk, cpos);
	chunk_mesh_clear(chunk);

	long3_t newchunkblockpos = setheightmapfromcpos(context, chunk_pos_get(chunk));

	static const block_t water = {
		.id = WATER,
		{
			.number = SIM_WATER_LEVELS
		}
	};

	int x, y, z;
	for(x=0; x<CHUNKSIZE; ++x)
	for(z=0; z<CHUNKSIZE; ++z)
	for(y=0; y<CHUNKSIZE; ++y)
	{
		double height = getheightval(context, x, z);
		int32_t blockheight = y + newchunkblockpos.y;
		if(blockheight < height - 100)
			chunk_block_set_id(chunk, x, y, z, BEDROCK);
		else if(blockheight < height - 20)
			chunk_block_set_id(chunk, x, y, z, STONE);
		else if(blockheight < height - 3)
			chunk_block_set_id(chunk, x, y, z, DIRT);
		else if(blockheight < height)
			if(height < .55)
                if(newchunkblockpos.x+x <= 6 && newchunkblockpos.x+x >= -6)
                    if(newchunkblockpos.x+x == 5 || newchunkblockpos.x+x == -5)
                        chunk_block_set_id(chunk, x, y, z, LINE_WHITE);
                    else if (newchunkblockpos.x+x == 0)
                        chunk_block_set_id(chunk, x, y, z, LINE_YELLOW);
                    else
                        chunk_block_set_id(chunk, x, y, z, ROAD);
                else
                    chunk_block_set_id(chunk, x, y, z, SAND);
			else
				chunk_block_set_id(chunk, x, y, z, GRASS);
		else if(blockheight < 0)
			chunk_block_set(chunk, x, y, z, water);
		else
			break;
	}

	chunk_unlock(chunk);
}

long
worldgen_get_height_of_pos(worldgen_t *context, long x, long z)
{
	if(context == 0)
		context = &defaultcontext;

	setheightmapfromcpos(context, world_get_chunkpos_of_worldpos(x,0,z));
	int3_t internalpos = world_get_internalpos_of_worldpos(x,0,z);
	return floor(getheightval(context, internalpos.x, internalpos.z));
}
