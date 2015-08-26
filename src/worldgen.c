#include "worldgen.h"

void
worldgen_genchunk(chunk_t *chunk)
{
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
					chunk_setblockid(chunk, x, y, z, BLOCK_ID_STONE);
				else
					chunk_setair(chunk, x, y, z);
			}
		}
	}
}

