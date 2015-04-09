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

	int termscreenuvsize;
	GLfloat *termscreenuvdata;
} mesh_t;

#endif //MESH_H
