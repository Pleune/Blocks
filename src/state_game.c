#include "state_game.h"

#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "state.h"
#include "custommath.h"

uint32_t ticks = 0;

GLuint vertexbuffer;
GLuint program;
GLuint matrix;

int fpscap = 1;
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

	matrix = glGetUniformLocation(program, "MVP");
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
		else if(e.type == SDL_WINDOWEVENT)
		{
			if(e.window.event == SDL_WINDOWEVENT_RESIZED)
				updatewindowbounds(e.window.data1, e.window.data2);
		}
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	int windoww, windowh;
	getwindowsize(&windoww, &windowh);

	mat4_t projection = getprojectionmatrix(90, (float)windoww / (float)windowh, .1, 100);

	static float i = 4;
	static int wobbledir = 0;
	if(wobbledir)
	{
		i+= .3;
		if(i>6)
			wobbledir = 0;
	} else {
		i -= .1;
		if(i<-6)
			wobbledir = 1;
	}

	vec3_t pos;
	pos.x = i;
	pos.y = 4;
	pos.z = 3;
	vec3_t target;
	target.x = 0;
	target.y = 0;
	target.z = 0;
	vec3_t up;
	up.x = 0;
	up.y = 1;
	up.z = 0;
	mat4_t view = getviewmatrix(pos, target, up);

	mat4_t mvp;
	dotmat4mat4(&mvp, &projection, &view);
	glUniformMatrix4fv(matrix, 1, GL_FALSE, mvp.mat);

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

	swapwindow();

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
