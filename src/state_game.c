#include "state_game.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "gl.h"
#include "state.h"
#include "custommath.h"
#include "world.h"
#include "blockpick.h"
#include "debug.h"

int windoww, windowh;

GLuint drawprogram;
GLuint drawprogrammatrixinput;

GLuint ppprogram;
GLuint pppointbuffer;
GLuint pppointbufferid;

struct {
	GLuint framebuffer;

	GLuint colorbuffer;
	GLuint depthbuffer;

	GLuint colorbufferid;
	GLuint depthbufferid;
} renderbuffer;

vec3_t forwardcamera;

const static vec3_t up = {0,1,0};

int fpscap = 1;
const int fpsmax = 120;

vec3_t pos;
float rotx, roty;

int lines = 0;
int pp = 1;
int takeinput = 1;

void
state_game_init()
{
	getwindowsize(&windoww, &windowh);

	//load shaders 'n stuff
	gl_loadprogram(&drawprogram, "shaders/vs", "shaders/fs");
	gl_loadprogram(&ppprogram, "shaders/pvs", "shaders/pfs");

	drawprogrammatrixinput = glGetUniformLocation(drawprogram, "MVP");
	pppointbufferid = glGetUniformLocation(ppprogram, "tex");

	//generate the post processing framebuffer
	glGenFramebuffers(1, &renderbuffer.framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, renderbuffer.framebuffer);

	glGenTextures(1, &renderbuffer.colorbuffer);
	glGenTextures(1, &renderbuffer.depthbuffer);

	glBindTexture(GL_TEXTURE_2D, renderbuffer.colorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windoww, windowh, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, renderbuffer.depthbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, windoww, windowh, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderbuffer.colorbuffer, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderbuffer.depthbuffer, 0);

	//put the textures in thr right spots
	glUseProgram(ppprogram);
	GLuint tex = glGetUniformLocation(ppprogram, "tex");
	GLuint depth = glGetUniformLocation(ppprogram, "depth");

	glUniform1i(tex, 0);
	glUniform1i(depth, 1);

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
		fail("Bad Framebuffer");

	pos.x = 0;
	pos.y = 0;
	pos.z = 3;
	rotx = 0;
	roty = 0;
	world_init(pos);

	centermouse();
	SDL_ShowCursor(0);
}

void
update(uint32_t dt)
{
}

void
input(uint32_t dt)
{

	SDL_PumpEvents();

	const uint8_t *keyboard = SDL_GetKeyboardState(0);

	if(takeinput)
	{
		int mousex, mousey;
		SDL_GetMouseState(&mousex, &mousey);
		centermouse();
		double deltamousex = mousex - windoww/2;
		double deltamousey = mousey - windowh/2;

		rotx += deltamousex/800;
		roty -= deltamousey/800;
	}

	forwardcamera.x = sin(rotx) * cos(roty);
	forwardcamera.y = sin(roty) + 0.4f;
	forwardcamera.z = -cos(rotx) * cos(roty);

	vec2_t forwardmovement;
	forwardmovement.x = sin(rotx);
	forwardmovement.y = -cos(rotx);

	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		if(e.type == SDL_QUIT)
			state_changeto(CLOSING);

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
				case SDLK_m:
					takeinput = !takeinput;
				break;
				case SDLK_p:
					pp = !pp;
					if(!pp)
					{
						glBindFramebuffer(GL_FRAMEBUFFER, 0);
						glEnable(GL_DEPTH_TEST);
						glUseProgram(drawprogram);
					}
				break;

			}
		}
		else if(e.type == SDL_MOUSEBUTTONDOWN)
		{
			if(e.button.button == SDL_BUTTON_LEFT)
			{
				block_t b;
				b.id = 2;
				game_rayadd(pos, forwardcamera, b, 1);
			}
			else if(e.button.button == SDL_BUTTON_RIGHT)
			{
				game_raydel(pos, forwardcamera);
			}
		}
		else if(e.type == SDL_WINDOWEVENT)
		{
			if(e.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				updatewindowbounds(e.window.data1, e.window.data2);
				windoww = e.window.data1;
				windowh = e.window.data2;
				centermouse();

				glBindFramebuffer(GL_FRAMEBUFFER, renderbuffer.framebuffer);

				glBindTexture(GL_TEXTURE_2D, renderbuffer.colorbuffer);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windoww, windowh, 0, GL_RGB, GL_FLOAT, 0);

				glBindTexture(GL_TEXTURE_2D, renderbuffer.depthbuffer);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, windoww, windowh, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

				if(!pp)
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}
	}
#define SPEED 50
	if(keyboard[SDL_SCANCODE_W])
	{
		pos.x += SPEED * forwardmovement.x * (dt / 1000.0);
		pos.z += SPEED * forwardmovement.y * (dt / 1000.0);
	}
	if(keyboard[SDL_SCANCODE_A])
	{
		pos.x += SPEED * forwardmovement.y * (dt / 1000.0);
		pos.z -= SPEED * forwardmovement.x * (dt / 1000.0);
	}
	if(keyboard[SDL_SCANCODE_S])
	{
		pos.x -= SPEED * forwardmovement.x * (dt / 1000.0);
		pos.z -= SPEED * forwardmovement.y * (dt / 1000.0);
	}
	if(keyboard[SDL_SCANCODE_D])
	{
		pos.x -= SPEED * forwardmovement.y * (dt / 1000.0);
		pos.z += SPEED * forwardmovement.x * (dt / 1000.0);
	}
	if(keyboard[SDL_SCANCODE_LSHIFT])
	{
		pos.y -= SPEED * (dt / 1000.0);
	}
	if(keyboard[SDL_SCANCODE_SPACE])
	{
		pos.y += SPEED * (dt / 1000.0);
	}

	forwardcamera.x += pos.x;
	forwardcamera.y += pos.y;
	forwardcamera.z += pos.z;
}

void
render(uint32_t dt)
{
	static uint32_t oneseccond = 0;
	oneseccond += dt;
	static int frame = 0;
	frame++;
	if(oneseccond >= 1000)
	{
		oneseccond -= 1000;
		info("FPS: %i", frame);
		frame=0;
	}

	input(dt);

	mat4_t projection = getprojectionmatrix(90, (float)windoww / (float)windowh, 3000, .1);
	mat4_t view = getviewmatrix(pos, forwardcamera, up);

	mat4_t mvp;
	dotmat4mat4(&mvp, &projection, &view);

	//render to framebuffer here
	if(pp)
		glBindFramebuffer(GL_FRAMEBUFFER, renderbuffer.framebuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(lines)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glUseProgram(drawprogram);
	glUniformMatrix4fv(drawprogrammatrixinput, 1, GL_FALSE, mvp.mat);
	world_render(pos);

	if(lines)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//render to screen here
	if(pp)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(ppprogram);
		glBindBuffer(GL_ARRAY_BUFFER, pppointbuffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderbuffer.colorbuffer);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, renderbuffer.depthbuffer);

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	swapwindow();
}

void
state_game_run()
{
	static uint32_t ticks = 0;
	uint32_t newticks = SDL_GetTicks();
	uint32_t deltatime = ticks ? newticks - ticks : 0;
	ticks = newticks;

	static uint32_t updatebuild = 0;
	updatebuild += deltatime;
	if(updatebuild >= 500)
	{
		updatebuild -= 500;
		update(500);
	}
	render(deltatime);

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
	glDeleteFramebuffers(1, &renderbuffer.framebuffer);
	glDeleteTextures(1, &renderbuffer.colorbuffer);
	glDeleteTextures(1, &renderbuffer.depthbuffer);
	world_cleanup();
}
