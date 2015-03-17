#include "world.h"

#include <stdio.h>
#include <GL/glew.h>

#include "defines.h"
#include "chunk.h"

chunk_t loadedchunks[WORLDSIZE][WORLDSIZE][WORLDSIZE];

struct {
	GLuint vao;
	GLuint vbo;
	int iscurrent;
	long points;
} blockvbos[WORLDSIZE][WORLDSIZE][WORLDSIZE];

void
world_initalload()
{
	int x, y, z;
	for(x=0; x<WORLDSIZE; x++)
	{
		for(y=0; y<WORLDSIZE; y++)
		{
			for(z=0; z<WORLDSIZE; z++)
			{
				glGenBuffers(1, &blockvbos[x][y][z].vbo);
				glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].vbo);

				blockvbos[x][y][z].points = 0;
				blockvbos[x][y][z].iscurrent = 0;

				loadedchunks[x][y][z] = callocchunk();
				loadedchunks[x][y][z].pos[0] = x;
				loadedchunks[x][y][z].pos[1] = y;
				loadedchunks[x][y][z].pos[2] = z;

				loadedchunks[x][y][z].data[0].id = 1;//to a refrence point when rendering
			}
		}
	}
}

void
world_render()
{
	int x=0;
	int y=0;
	int z=0;

	for(x=0; x<WORLDSIZE; x++)
	{
		for(y=0; y<WORLDSIZE; y++)
		{
			for(z=0; z<WORLDSIZE; z++)
			{
				glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].vbo);
				glVertexAttribPointer(
						0,
						3,
						GL_FLOAT,
						GL_FALSE,
						0,
						0);
				glEnableVertexAttribArray(0);
				if(!blockvbos[x][y][z].iscurrent)
				{
					mesh_t mesh = chunk_getmesh(loadedchunks[x][y][z], 0,0,0,0,0,0);

					glBufferData(GL_ARRAY_BUFFER, mesh.size * sizeof(GLfloat), mesh.data, GL_STATIC_DRAW);

					blockvbos[x][y][z].points = mesh.size / 3;
					blockvbos[x][y][z].iscurrent = 1;
				}

				glDrawArrays(GL_TRIANGLES, 0, blockvbos[x][y][z].points);
			}
		}
	}
}
