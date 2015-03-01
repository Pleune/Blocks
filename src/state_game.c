#include "state_game.h"

#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "state.h"

uint32_t ticks = 0;

GLuint vertexbuffer;
GLuint program;

int fpscap = 0;
const int fpsmax = 60;

static void
fail(char* msg)
{
	printf("ERROR: %s", msg);
	state_changeto(CLOSING);
}

void
state_game_init()
{
	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &vertexbuffer);

	//load shaders 'n stuff

	GLuint vertexshader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);

	char *file;
	char *file_contents;
	long input_file_size;
	FILE *input_file;
	const char *intermediary;

	file = (char *)calloc(1024, sizeof(char));
	strncat(file, getbasepath(), 1024);
	strncat(file, "shaders/vertexshader", 1024 - strlen(file));

	input_file = fopen(file, "rb");
	free(file);
	if(!input_file)
		fail("getting a handle on the vertexshader file");
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
	strncat(file, "shaders/fragmentshader", 1024 - strlen(file));

	input_file = fopen(file, "rb");
	free(file);
	if(!input_file)
		fail("getting a handle on the fragmentshader file");
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

	program = glCreateProgram();
	glAttachShader(program, vertexshader);
	glAttachShader(program, fragmentshader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &errorlen);
	message = (char *)malloc(errorlen * sizeof(char));
	glGetProgramInfoLog(program, errorlen, 0, message);
	printf("INFO: opengl program info:\n%s\n", message);
	free(message);

	glDeleteShader(vertexshader);
	glDeleteShader(fragmentshader);
}

void
state_game_run()
{
	ticks = SDL_GetTicks();

	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		//handle input
		if(e.type == SDL_KEYDOWN)
		{
			switch(e.key.keysym.sym)
			{
				case SDLK_ESCAPE:
					state_changeto(CLOSING);
				break;
			}
		}
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	GLfloat g_vertex_buffer_data[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
	};

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	//Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle

	glDisableVertexAttribArray(0);

	swapWindow();

	if(fpscap)
	{
		uint32_t ticksdiff = SDL_GetTicks() - ticks;
		if(ticksdiff < 1000 / fpsmax)
			SDL_Delay(1000 / fpsmax - ticksdiff);
	}
}

void
state_game_close()
{

}
