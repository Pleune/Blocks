#ifndef MESH_H
#define MESH_H

#include <GL/glew.h>

typedef struct {
	long size;
	GLfloat *data;
	long colorsize;
	GLfloat *colordata;
} mesh_t;

#endif //MESH_H
