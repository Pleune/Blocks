#include "worldgen.h"

#include <math.h>
#include <limits.h>
#include "world.h"
#include "custommath.h"
#include "defines.h"
#include "modulo.h"

#define DIAMONDSQUARESIZE (int) CAT(0x1p, DIAMONDSQUARELEVELS)

long3_t lastchunkblockpos = {LONG_MAX, LONG_MAX, LONG_MAX};
long3_t lastdiasquareblockpos = {LONG_MAX, LONG_MAX, LONG_MAX};
double heightmap[(CHUNKSIZE+1)*(CHUNKSIZE+1)];
double metaheightmap[(DIAMONDSQUARESIZE+1)*(DIAMONDSQUARESIZE+1)];

static uint32_t
hash( uint32_t a)
{
	a = (a+0x7ed55d16) + (a<<12);
	a = (a^0xc761c23c) ^ (a>>19);
	a = (a+0x165667b1) + (a<<5);
	a = (a+0xd3a2646c) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	return a;
}

static uint32_t
noise(uint32_t x, uint32_t y, uint32_t seed)
{
	return hash(((hash(x)<<16) ^ hash(y)) + seed);
}

/**
 * flattens the world out around y = 0
 */
void
bias(double *data)
{
	int x, z;
	for(x=0; x<CHUNKSIZE+1; x++)
	{
		for(z=0; z<CHUNKSIZE+1; z++)
		{
			int index = x + z*(CHUNKSIZE+1);
			double h = data[index];

			data[index] = (h*h*h*h*h)/(h*h*h*h+300*h*h);
		}
	}
}

double
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
void
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
				data[x + z*size] += (double)((noise(pos.x + x*scale, pos.z + z*scale, seed)%100)/100.0 - weight(data[x + z*size]))*r;
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
				data[x + z*size] += (double)((noise(pos.x + x*scale, pos.z + z*scale, seed)%100)/100.0 - weight(data[x + z*size]))*r;
				data[z + x*size] /= notedge ? 4.0 : 2.0;
				data[z + x*size] += (double)((noise(pos.x + z*scale, pos.z + x*scale, seed)%100)/100.0 - weight(data[z + x*size]))*r;
			}
		}
	}
}

long3_t
setheightmapfromcpos(long3_t cpos)
{
	uint32_t seed = world_getseed();

	long3_t newchunkblockpos;
	newchunkblockpos.x = cpos.x * CHUNKSIZE;
	newchunkblockpos.y = cpos.y * CHUNKSIZE;
	newchunkblockpos.z = cpos.z * CHUNKSIZE;

	if(lastchunkblockpos.x != newchunkblockpos.x || lastchunkblockpos.z != newchunkblockpos.z || (lastchunkblockpos.x == LONG_MAX || lastchunkblockpos.z == LONG_MAX))
	{
		lastchunkblockpos = newchunkblockpos;
		long3_t newdiasquareblockpos = {
			floor((double)cpos.x / (double)DIAMONDSQUARESIZE) * DIAMONDSQUARESIZE * CHUNKSIZE,
			floor((double)cpos.y / (double)DIAMONDSQUARESIZE) * DIAMONDSQUARESIZE * CHUNKSIZE,
			floor((double)cpos.z / (double)DIAMONDSQUARESIZE) * DIAMONDSQUARESIZE * CHUNKSIZE
		};

		if(lastdiasquareblockpos.x != newdiasquareblockpos.x || lastdiasquareblockpos.z != newdiasquareblockpos.z || (lastdiasquareblockpos.x == LONG_MAX || lastdiasquareblockpos.z == LONG_MAX))
		{
			lastdiasquareblockpos = newdiasquareblockpos;

			metaheightmap[0] =
				((noise(
					newdiasquareblockpos.x,
					newdiasquareblockpos.z,
					seed
					)%100)/100.0 - .5) * (DIAMONDSQUARESIZE*WORLDGEN_RANGE);
			metaheightmap[DIAMONDSQUARESIZE] =
				((noise(
					newdiasquareblockpos.x + DIAMONDSQUARESIZE*CHUNKSIZE,
					newdiasquareblockpos.z,
					seed
					)%100)/100.0 - .5) * (DIAMONDSQUARESIZE*WORLDGEN_RANGE);
			metaheightmap[(DIAMONDSQUARESIZE+1)*DIAMONDSQUARESIZE] =
				((noise(
					newdiasquareblockpos.x,
					newdiasquareblockpos.z + DIAMONDSQUARESIZE*CHUNKSIZE,
					seed
					)%100)/100.0 - .5) * (DIAMONDSQUARESIZE*WORLDGEN_RANGE);
			metaheightmap[DIAMONDSQUARESIZE + (DIAMONDSQUARESIZE+1)*DIAMONDSQUARESIZE] =
				((noise(
					newdiasquareblockpos.x + DIAMONDSQUARESIZE*CHUNKSIZE,
					newdiasquareblockpos.z + DIAMONDSQUARESIZE*CHUNKSIZE,
					seed
					)%100)/100.0 - .5) * (DIAMONDSQUARESIZE*WORLDGEN_RANGE);

			pound(metaheightmap, DIAMONDSQUARESIZE+1, newdiasquareblockpos, seed, CHUNKSIZE, DIAMONDSQUARELEVELS);
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

		pound(heightmap, CHUNKSIZE+1, newchunkblockpos, seed, 1, CHUNKLEVELS);
		bias(heightmap);
	}
	return newchunkblockpos;
}

double
getheightval(long x, long z)
{
	return (heightmap[
				x+1 + z*(CHUNKSIZE+1)] +
				heightmap[x + z*(CHUNKSIZE+1)] +
				heightmap[x+1 + (z+1)*(CHUNKSIZE+1)] +
				heightmap[x + (z+1)*(CHUNKSIZE+1)
			]) / 4.0;
}

void
worldgen_genchunk(chunk_t *chunk)
{
	long3_t newchunkblockpos = setheightmapfromcpos(chunk_getpos(chunk));

	static const block_t water = {
		.id = WATER,
		{
			.number = SIM_WATER_LEVELS
		}
	};

	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	{
		for(z=0; z<CHUNKSIZE; z++)
		{
			for(y=0; y<CHUNKSIZE; y++)
			{
				double height = getheightval(x,z);
				int32_t blockheight = y + newchunkblockpos.y;
				if(blockheight < height - 100)
					chunk_setblockid(chunk, x, y, z, BEDROCK);
				else if(blockheight < height - 20)
					chunk_setblockid(chunk, x, y, z, STONE);
				else if(blockheight < height - 3)
					chunk_setblockid(chunk, x, y, z, DIRT);
				else if(blockheight < height)
					if(height < .55)
						chunk_setblockid(chunk, x, y, z, SAND);
					else
						chunk_setblockid(chunk, x, y, z, GRASS);
				else if(blockheight < 0)
					chunk_setblock(chunk, x, y, z, water);
				else
					break;
			}
		}
	}
}

long
worldgen_getheightfrompos(long x, long z)
{
	setheightmapfromcpos(world_getchunkposofworldpos(x,0,z));
	int3_t internalpos = world_getinternalposofworldpos(x,0,z);
	return floor(getheightval(internalpos.x, internalpos.z));
}
