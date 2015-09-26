#include "worldgen.h"

#include <math.h>
#include <limits.h>
#include "world.h"
#include "custommath.h"
#include "defines.h"
#include "modulo.h"

#define DIAMONDSQUARESIZE (int) CAT(0x1p, DIAMONDSQUARELEVELS)

uint32_t hash( uint32_t a)
{
	a = (a+0x7ed55d16) + (a<<12);
	a = (a^0xc761c23c) ^ (a>>19);
	a = (a+0x165667b1) + (a<<5);
	a = (a+0xd3a2646c) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	return a;
}

uint32_t noise(uint32_t x, uint32_t y, uint32_t seed)
{
	return hash(seed+hash(x+hash(y)));
}

void
bias(double *data, size_t size)
{
	int x, z;
	for(x=0; x<size; x++)
	{
		for(z=0; z<size; z++)
		{
			int index = x + z*size;
			double h = data[index];

				data[index] = (h*h*h*h*h)/(h*h*h*h+300*h*h);
		}
	}
}

void
pound(double *data, size_t size, int step, long3_t pos, uint32_t seed, int scale)
{
	//lines
	int x, z;
	int d_ = step == 0 ? 1 : pow(2, step);
	int d = d_ * 2;

	int r = d_ * scale * 10.0 * BUMPYNESS;
	int b = r / 20.0;

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
				) / 4.0 + (double)(noise(pos.x + x*scale, pos.z + z*scale, seed)%r)/10.0 - b;
		}
	}

	for(z=0; z<size; z+=d)
	{
		for(x=d_; x<size; x+=d)
		{
			int four = (z-d_>=0) && (z+d_ <size);
			data[x + z*size] = (data[x-d_ + z*size] + data[x+d_ + z*size]);
			data[z + x*size] = (data[z + (x-d_)*size] + data[z + (x+d_)*size]);
			if(four)
			{
				data[x + z*size] += data[x + (z-d_)*size];
				data[z + x*size] += data[z-d_ + x*size];
				data[x + z*size] += data[x + (z+d_)*size];
				data[z + x*size] += data[z+d_ + x*size];
			}
			data[x + z*size] /= four ? 4.0 : 2.0;
			data[x + z*size] += (double)(noise(pos.x + x*scale, pos.z + z*scale, seed)%r)/10.0 - b;
			data[z + x*size] /= four ? 4.0 : 2.0;
			data[z + x*size] += (double)(noise(pos.x + z*scale, pos.z + x*scale, seed)%r)/10.0 - b;
		}
	}
}

void
worldgen_genchunk(chunk_t *chunk)
{
	int i;

	uint32_t seed = world_getseed();

	static long3_t lastchunkblockpos = {LONG_MAX, LONG_MAX, LONG_MAX};
	long3_t cpos = chunk_getpos(chunk);
	long3_t newchunkblockpos = chunk_getpos(chunk);
	newchunkblockpos.x = cpos.x * CHUNKSIZE;
	newchunkblockpos.y = cpos.y * CHUNKSIZE;
	newchunkblockpos.z = cpos.z * CHUNKSIZE;

	static double heightmap[(CHUNKSIZE+1)*(CHUNKSIZE+1)];

	if(lastchunkblockpos.x != newchunkblockpos.x || lastchunkblockpos.z != newchunkblockpos.z || (lastchunkblockpos.x == LONG_MAX || lastchunkblockpos.z == LONG_MAX))
	{
		lastchunkblockpos = newchunkblockpos;

		static double metaheightmap[(DIAMONDSQUARESIZE+1)*(DIAMONDSQUARESIZE+1)];

		static long3_t lastdiasquareblockpos = {LONG_MAX, LONG_MAX, LONG_MAX};
		long3_t newdiasquareblockpos = {
			floor((double)cpos.x / (double)DIAMONDSQUARESIZE) * DIAMONDSQUARESIZE * CHUNKSIZE,
			floor((double)cpos.y / (double)DIAMONDSQUARESIZE) * DIAMONDSQUARESIZE * CHUNKSIZE,
			floor((double)cpos.z / (double)DIAMONDSQUARESIZE) * DIAMONDSQUARESIZE * CHUNKSIZE
		};

		if(lastdiasquareblockpos.x != newdiasquareblockpos.x || lastdiasquareblockpos.z != newdiasquareblockpos.z || (lastdiasquareblockpos.x == LONG_MAX || lastdiasquareblockpos.z == LONG_MAX))
		{
			lastdiasquareblockpos = newdiasquareblockpos;

			metaheightmap[0									] =
				(noise(newdiasquareblockpos.x					, newdiasquareblockpos.z				,1)%(DIAMONDSQUARESIZE*10*BUMPYNESS)) / 10.0;
			metaheightmap[DIAMONDSQUARESIZE							] =
				(noise(newdiasquareblockpos.x + DIAMONDSQUARESIZE*CHUNKSIZE	, newdiasquareblockpos.z				,1)%(DIAMONDSQUARESIZE*10*BUMPYNESS)) / 10.0;
			metaheightmap[				(DIAMONDSQUARESIZE+1)*DIAMONDSQUARESIZE	] =
				(noise(newdiasquareblockpos.x					, newdiasquareblockpos.z + DIAMONDSQUARESIZE*CHUNKSIZE	,1)%(DIAMONDSQUARESIZE*10*BUMPYNESS)) / 10.0;
			metaheightmap[DIAMONDSQUARESIZE + 	(DIAMONDSQUARESIZE+1)*DIAMONDSQUARESIZE	] =
				(noise(newdiasquareblockpos.x + DIAMONDSQUARESIZE*CHUNKSIZE	, newdiasquareblockpos.z + DIAMONDSQUARESIZE*CHUNKSIZE	,1)%(DIAMONDSQUARESIZE*10*BUMPYNESS)) / 10.0;

			for(i = DIAMONDSQUARELEVELS-1; i>=0; i--)
			{
				pound(metaheightmap, DIAMONDSQUARESIZE+1, i, newdiasquareblockpos, seed, CHUNKSIZE);
			}
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

		for(i = CHUNKLEVELS-1; i>=0; i--)
		{
			pound(heightmap, CHUNKSIZE+1, i, newchunkblockpos, seed, 1);
		}
		bias(heightmap, CHUNKSIZE+1);
	}

	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	{
		for(z=0; z<CHUNKSIZE; z++)
		{
			for(y=0; y<CHUNKSIZE; y++)
			{
				double height = (heightmap[x+1 + z*(CHUNKSIZE+1)] +
						heightmap[x + z*(CHUNKSIZE+1)] +
						heightmap[x+1 + (z+1)*(CHUNKSIZE+1)] +
						heightmap[x + (z+1)*(CHUNKSIZE+1)]) / 4.0;
				int32_t blockheight = y + newchunkblockpos.y;
				if(blockheight < height - 3)
					chunk_setblockid(chunk, x, y, z, STONE);
				else if(blockheight < height)
					if(height < .55)
						chunk_setblockid(chunk, x, y, z, SAND);
					else
						chunk_setblockid(chunk, x, y, z, GRASS);
				else if(blockheight < 0)
					chunk_setblockid(chunk, x, y, z, WATER);
				else
					break;
			}
		}
	}
}

