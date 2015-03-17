#include "state_game.h"

#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "state.h"
#include "custommath.h"

#include "world.h"

uint32_t ticks = 0;

int windoww, windowh;

GLuint drawprogram;
GLuint ppprogram;

GLuint matrix;

GLuint pppointbuffer;
GLuint pppointbifferid;

struct {
	GLuint framebuffer;
	GLuint renderbuffer;
} framebuffer;
GLuint ppinputtex;


int fpscap = 0;
const int fpsmax = 60;

vec3_t pos;
float rotx, roty;

int lines = 0;

static void
fail(char* msg)
{
	printf("ERROR: %s\n", msg);
	state_changeto(CLOSING);
}

static void
loadprogram(GLuint *program, char *vertexshadername, char *fragmentshadername)
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
	strncat(file, fragmentshadername, 1024 - strlen(file));

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

void
state_game_init()
{
	getwindowsize(&windoww, &windowh);

	//load shaders 'n stuff
	loadprogram(&drawprogram, "vs", "fs");
	loadprogram(&ppprogram, "pvs", "pfs");

	matrix = glGetUniformLocation(drawprogram, "MVP");
	pppointbifferid = glGetUniformLocation(ppprogram, "tex");

	//generate the post processing framebuffer
	glGenFramebuffers(1, &framebuffer.framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.framebuffer);

	glGenTextures(1, &ppinputtex);
	glBindTexture(GL_TEXTURE_2D, ppinputtex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windoww, windowh, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenRenderbuffers(1, &framebuffer.renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, windoww, windowh);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer.renderbuffer);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ppinputtex, 0);

	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers);

	//generate the post processing mesh
	GLfloat data[] = {
		-1.0f, 1.0f,
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f,
		1.0f, -1.0f
	};

	glGenBuffers(1, &pppointbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, pppointbuffer);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), data, GL_STATIC_DRAW);


	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("fail\n");
		//the above failed
		//TODO: fix the framebuffer, dont use it,
	}

	//load the world
	world_initalload();

	pos.x = 0;
	pos.y = 0;
	pos.z = 3;
	rotx = 0;
	roty = 0;

	centermouse();
	SDL_ShowCursor(0);

	ticks = SDL_GetTicks();
}

void
state_game_run()
{
	int newticks = SDL_GetTicks();
	double deltatime = newticks - ticks;
	ticks = newticks;

	static uint32_t lastcheck = 0;
	static int frame = 0;
	frame++;
	if(newticks - lastcheck >= 1000)
	{
		lastcheck = newticks;
		printf("FPS: %i\n", frame);
		frame=0;
	}

	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		if(e.type == SDL_QUIT)
			state_changeto(CLOSING);
		//handle input
		if(e.type == SDL_KEYDOWN)
		{
			switch(e.key.keysym.sym)
			{
				case SDLK_ESCAPE:
					state_changeto(CLOSING);
				break;
				case SDLK_v:
					lines = !lines;
				break;
			}
		}
		else if(e.type == SDL_WINDOWEVENT)
		{
			if(e.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				updatewindowbounds(e.window.data1, e.window.data2);
				windoww = e.window.data1;
				windowh = e.window.data2;
				glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.renderbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, windoww, windowh);

				glDeleteTextures(1, &ppinputtex);

				glGenTextures(1, &ppinputtex);
				glBindTexture(GL_TEXTURE_2D, ppinputtex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windoww, windowh, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.framebuffer);

				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ppinputtex, 0);
			}
		}
	}

	const uint8_t *keyboard = SDL_GetKeyboardState(0);
	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);
	centermouse();
	double deltamousex = mousex - windoww/2;
	double deltamousey = mousey - windowh/2;

	mat4_t projection = getprojectionmatrix(90, (float)windoww / (float)windowh, .1, 2000);

	vec3_t up;
	up.x = 0;
	up.y = 1;
	up.z = 0;

	rotx += deltamousex/800;
	roty -= deltamousey/800;

	vec3_t forwardcamera;
	forwardcamera.x = sin(rotx) * cos(roty);
	forwardcamera.y = sin(roty);
	forwardcamera.z = -cos(rotx) * cos(roty);

	vec2_t forwardmovement;
	forwardmovement.x = sin(rotx);
	forwardmovement.y = -cos(rotx);

	vec2_t right;
	right.x = -forwardcamera.y;
	right.y = -forwardcamera.x;

	if(keyboard[SDL_SCANCODE_W])
	{
		pos.x += 14 * forwardmovement.x * (deltatime / 1000);
		pos.z += 14 * forwardmovement.y * (deltatime / 1000);
	}
	if(keyboard[SDL_SCANCODE_A])
	{
		pos.x += 14 * forwardmovement.y * (deltatime / 1000);
		pos.z -= 14 * forwardmovement.x * (deltatime / 1000);
	}
	if(keyboard[SDL_SCANCODE_S])
	{
		pos.x -= 14 * forwardmovement.x * (deltatime / 1000);
		pos.z -= 14 * forwardmovement.y * (deltatime / 1000);
	}
	if(keyboard[SDL_SCANCODE_D])
	{
		pos.x -= 14 * forwardmovement.y * (deltatime / 1000);
		pos.z += 14 * forwardmovement.x * (deltatime / 1000);
	}
	if(keyboard[SDL_SCANCODE_LSHIFT])
	{
		pos.y -= 14 * (deltatime / 1000);
	}
	if(keyboard[SDL_SCANCODE_SPACE])
	{
		pos.y += 14 * (deltatime / 1000);
	}

	forwardcamera.x += pos.x;
	forwardcamera.y += pos.y;
	forwardcamera.z += pos.z;

	mat4_t view = getviewmatrix(pos, forwardcamera, up);

	mat4_t mvp;
	dotmat4mat4(&mvp, &projection, &view);

	//render to framebuffer here
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.framebuffer);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(drawprogram);

	if(lines)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glUniformMatrix4fv(matrix, 1, GL_FALSE, mvp.mat);

	world_render();

	if(lines)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//render to screen here
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(ppprogram);
	glBindBuffer(GL_ARRAY_BUFFER, pppointbuffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ppinputtex);
	glUniform1i(pppointbifferid, 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

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
	glDeleteProgram(drawprogram);
	glDeleteFramebuffers(1, &framebuffer.framebuffer);
	glDeleteRenderbuffers(1, &framebuffer.renderbuffer);
}
