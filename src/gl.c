#include "gl.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "state.h"
#include "debug.h"

void
gl_program_load_str(GLuint *program, const char *vertexshader_src, const char *fragmentshader_src)
{
	GLuint vertexshader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexshader, 1, &vertexshader_src, NULL);
	glCompileShader(vertexshader);

	glShaderSource(fragmentshader, 1, &fragmentshader_src, NULL);
	glCompileShader(fragmentshader);

	int errorlen;
	char *message;

	glGetShaderiv(vertexshader, GL_INFO_LOG_LENGTH, &errorlen);
	if(errorlen)
	{
		message = (char *)malloc(errorlen * sizeof(char));
		glGetShaderInfoLog(vertexshader, errorlen, 0, message);
		printf("INFO: vertexshader info:\n%s\n", message);
		free(message);
	}

	glGetShaderiv(fragmentshader, GL_INFO_LOG_LENGTH, &errorlen);
	if(errorlen)
	{
		message = (char *)malloc(errorlen * sizeof(char));
		glGetShaderInfoLog(fragmentshader, errorlen, 0, message);
		printf("INFO: fragmentshader info:\n%s\n", message);
		free(message);
	}

	*program = glCreateProgram();
	glAttachShader(*program, vertexshader);
	glAttachShader(*program, fragmentshader);
	glLinkProgram(*program);

	glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &errorlen);
	if(errorlen)
	{
		message = (char *)malloc(errorlen * sizeof(char));
		glGetProgramInfoLog(*program, errorlen, 0, message);
		printf("INFO: opengl program info:\n%s\n", message);
		free(message);
	}

	glDeleteShader(vertexshader);
	glDeleteShader(fragmentshader);
}

void
gl_program_load_file(GLuint *program, char *vertexshadername, char *fragmentshadername)
{
	char *file;
	char *vertexshader_src;
	char *fragmentshader_src;
	long input_file_size;
	FILE *input_file;

	file = (char *)calloc(1024, sizeof(char));
	strncat(file, state_basepath_get(), 1024);
	strncat(file, vertexshadername, 1024 - strlen(file));

	input_file = fopen(file, "rb");
	free(file);
	if(!input_file)
		fail("getting a handle on the vertexshader %s", file);
	fseek(input_file, 0, SEEK_END);
	input_file_size = ftell(input_file);
	rewind(input_file);
	vertexshader_src = malloc((input_file_size + 1) * (sizeof(char)));
	fread(vertexshader_src, sizeof(char), input_file_size, input_file);
	fclose(input_file);
	vertexshader_src[input_file_size] = 0;

	file = (char *)calloc(1024, sizeof(char));
	strncat(file, state_basepath_get(), 1024);
	strncat(file, fragmentshadername, 1024 - strlen(file));

	input_file = fopen(file, "rb");
	free(file);
	if(!input_file)
		fail("getting a handle on the fragmentshader %s", file);
	fseek(input_file, 0, SEEK_END);
	input_file_size = ftell(input_file);
	rewind(input_file);
	fragmentshader_src = malloc((input_file_size + 1) * (sizeof(char)));
	fread(fragmentshader_src, sizeof(char), input_file_size, input_file);
	fclose(input_file);
	fragmentshader_src[input_file_size] = 0;

	info("loading %s and %s ...", vertexshadername, fragmentshadername);
	gl_program_load_str(program,vertexshader_src,fragmentshader_src);
	info("loading %s and %s complete", vertexshadername, fragmentshadername);

	free(vertexshader_src);
	free(fragmentshader_src);
}
