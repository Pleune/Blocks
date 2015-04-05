#include "worldgen.h"

#include "defines.h"

void
worldgen_genchunk(chunk_t *chunk)
{
	if(!chunk->data)
		free(chunk->data);
	chunk->data = chunk_callocdata();

	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	{
		for(y=0; y<CHUNKSIZE; y++)
		{
			for(z=0; z<CHUNKSIZE; z++)
			{
				int i = 0;
				if(x==0 || x==CHUNKSIZE-1)
					i++;
				if(y==0 || y==CHUNKSIZE-1)
					i++;
				if(z==0 || z==CHUNKSIZE-1)
					i++;
				if(i>1)
					chunk->data[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE].id=1;
			}
		}
	}
}
