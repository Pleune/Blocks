#include "gl.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "state.h"

void
gl_loadprogram(GLuint *program, char *vertexshadername, char *fragmentshadername)
{
	GLuint vertexshader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);

	char *file;
	char *file_contents;
	long input_file_size;
	FILE *input_file;
	const char *intermediary;

	file = (char *)calloc(1024, sizeof(char));
	strncat(file, getbasepath(), 1024);
	strncat(file, vertexshadername, 1024 - strlen(file));

	input_file = fopen(file, "rb");
	free(file);
	if(!input_file)
		printf("ERROR: getting a handle on the vertexshader file\n");
	fseek(input_file, 0, SEEK_END);
	input_file_size = ftell(input_file);
	rewind(input_file);
	file_contents = malloc((input_file_size + 1) * (sizeof(char)));
	fread(file_contents, sizeof(char), input_file_size, input_file);
	fclose(input_file);
	file_contents[input_file_size] = 0;

	intermediary = file_contents;
	glShaderSource(vertexshader, 1, &intermediary, NULL);
	glCompileShader(vertexshader);
	free(file_contents);

	file = (char *)calloc(1024, sizeof(char));
	strncat(file, getbasepath(), 1024);
	strncat(file, fragmentshadername, 1024 - strlen(file));

	input_file = fopen(file, "rb");
	free(file);
	if(!input_file)
		printf("ERROR: getting a handle on the fragmentshader file\n");
	fseek(input_file, 0, SEEK_END);
	input_file_size = ftell(input_file);
	rewind(input_file);
	file_contents = malloc((input_file_size + 1) * (sizeof(char)));
	fread(file_contents, sizeof(char), input_file_size, input_file);
	fclose(input_file);
	file_contents[input_file_size] = 0;

	intermediary = file_contents;
	glShaderSource(fragmentshader, 1, &intermediary, NULL);
	glCompileShader(fragmentshader);
	free(file_contents);

	int errorlen;
	char *message;

	glGetShaderiv(vertexshader, GL_INFO_LOG_LENGTH, &errorlen);
	message = (char *)malloc(errorlen * sizeof(char));
	glGetShaderInfoLog(vertexshader, errorlen, 0, message);
	printf("INFO: vertexshader info:\n%s\n", message);
	free(message);

	glGetShaderiv(fragmentshader, GL_INFO_LOG_LENGTH, &errorlen);
	message = (char *)malloc(errorlen * sizeof(char));
	glGetShaderInfoLog(fragmentshader, errorlen, 0, message);
	printf("INFO: fragmentshader info:\n%s\n", message);
	free(message);

	*program = glCreateProgram();
	glAttachShader(*program, vertexshader);
	glAttachShader(*program, fragmentshader);
	glLinkProgram(*program);

	glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &errorlen);
	message = (char *)malloc(errorlen * sizeof(char));
	glGetProgramInfoLog(*program, errorlen, 0, message);
	printf("INFO: opengl program info:\n%s\n", message);
	free(message);

	glDeleteShader(vertexshader);
	glDeleteShader(fragmentshader);
}
