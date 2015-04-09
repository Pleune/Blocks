#ifndef MESH_H
#define MESH_H

#include <GL/glew.h>

typedef struct {
	long vbosize;
	GLfloat *vbodata;

	long ebosize;
	GLuint *ebodata;

	long colorsize;
	GLfloat *colordata;

	int termscreensize;
	GLfloat *termscreendata;
} mesh_t;

#endif //MESH_H
