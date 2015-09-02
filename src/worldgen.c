#include "worldgen.h"

#include <math.h>
#include "custommath.h"

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
pound(double *data, size_t size, int step, long3_t cpos)
{
	//lines
	int x, z;
	int d_ = step == 0 ? 1 : pow(2, step);
	int d = d_ * 2;

	int r = d_ * 10.0;
	int b = r / 2.0;

	//square
	for(x=d_; x<size; x+=d)
	{
		for(z=d_; z<size; z+=d)
		{
			data[x + z*size] = (
					data[(x-d_) + (z-d_)*size] +
					//data[(x-d_) + (z)*size] +
					data[(x-d_) + (z+d_)*size] +
					//data[(x) + (z-d_)*size] +
					//data[(x) + (z+d_)*size] +
					data[(x+d_) + (z-d_)*size] +
					//data[(x+d_) + (z)*size] +
					data[(x+d_) + (z+d_)*size]
				) / 4.0 + (double)(noise(cpos.x + x, cpos.z + z, 1)%r) - b;
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
			data[x + z*size] += (double)(noise(cpos.x + x, cpos.z + z, 1)%r) - b;
			data[z + x*size] /= four ? 4.0 : 2.0;
			data[z + x*size] += (double)(noise(cpos.x + z, cpos.z + x, 1)%r) - b;
		}
	}
}

void
worldgen_genchunk(chunk_t *chunk)
{
	long3_t cpos = chunk_getpos(chunk);
	cpos.x *= CHUNKSIZE;
	cpos.y *= CHUNKSIZE;
	cpos.z *= CHUNKSIZE;

	double *heightmap = calloc((CHUNKSIZE+1)*(CHUNKSIZE+1), sizeof(double));
	if(!heightmap)
	{
		fprintf(stderr,"error malloc heightmap\n");
		exit(-1);
	}


	int a = CHUNKSIZE;

	heightmap[0 + (CHUNKSIZE+1)*0] = (noise(cpos.x, cpos.z,1)%100) / 3.0;
	heightmap[a + (CHUNKSIZE+1)*0] = (noise(cpos.x + CHUNKSIZE, cpos.z,1)%100) / 3.0;
	heightmap[0 + (CHUNKSIZE+1)*a] = (noise(cpos.x, cpos.z + CHUNKSIZE,1)%100) / 3.0;
	heightmap[a + (CHUNKSIZE+1)*a] = (noise(cpos.x + CHUNKSIZE, cpos.z + CHUNKSIZE,1)%100) / 3.0;

	int i;
	for(i = CHUNKLEVELS-1; i>=0; i--)
	{
		pound(heightmap, CHUNKSIZE+1, i, cpos);
	}
	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	{
		for(y=0; y<CHUNKSIZE; y++)
		{
			for(z=0; z<CHUNKSIZE; z++)
			{
				if(y+cpos.y < (
						heightmap[x+1 + z*(CHUNKSIZE+1)] +
						heightmap[x + z*(CHUNKSIZE+1)] +
						heightmap[x+1 + (z+1)*(CHUNKSIZE+1)] +
						heightmap[x + (z+1)*(CHUNKSIZE+1)]) / 4.0)
					chunk_setblockid(chunk, x, y, z, BLOCK_ID_STONE);
				else
					chunk_setair(chunk, x, y, z);
			}
		}
	}
	free(heightmap);
}

