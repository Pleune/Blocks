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
#include "defines.h"
#include "entity.h"
#include "worldgen.h"

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
vec3_t headpos;

const static vec3_t up = {0,1,0};

int fpscap = 1;
const int fpsmax = 120;

entity_t *pos;
const vec3_t *posptr;
float rotx, roty;

int lines = 0;
int pp = 1;
int takeinput = 1;
int flying = 1;

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
	GLfloat mesh[] = {
		-1.0f, 1.0f,
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f,
		1.0f, -1.0f
	};

	glGenBuffers(1, &pppointbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, pppointbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mesh) * sizeof(GLfloat), mesh, GL_STATIC_DRAW);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		fail("Bad Framebuffer");

	
	rotx = 0;
	roty = 0;
	vec3_t zero = {0, 0, 0};
	world_init(zero);
	int spawnheight = worldgen_getheightfrompos(0, 0)+1;
	pos = entity_create(0, spawnheight, 0, .8, 1.9, 30);
	posptr = entity_getposptr(pos);
	vec3_t friction = {27, 27, 27};
	entity_setfriction(pos, friction);
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

		roty = roty > M_PI/2-.005 ? M_PI/2-.005 : roty;
		roty = roty < -M_PI/2+.005 ? -M_PI/2+.005 : roty;

		rotx = rotx > M_PI*2 ? rotx - M_PI*2: rotx;
		rotx = rotx < -M_PI*2 ? rotx + M_PI*2: rotx;
	}

	forwardcamera.x = sin(rotx) * cos(roty);
	forwardcamera.y = sin(roty);
	forwardcamera.z = -cos(rotx) * cos(roty);

	vec2_t forwardmovement;
	forwardmovement.x = sin(rotx);
	forwardmovement.y = -cos(rotx);

	vec3_t delta = {0,0,0};
	if(keyboard[SDL_SCANCODE_F])
	{
		flying = !flying;
	}
	if(keyboard[SDL_SCANCODE_T])
	{
		vec3_t top = *posptr;
		top.y = worldgen_getheightfrompos(posptr->x, posptr->z)+1;
		entity_setpos(pos, top);
	}
	
	if(flying)
	{
		if(keyboard[SDL_SCANCODE_W])
		{
			delta.x = SPEED * forwardmovement.x * (dt / 1000.0);
			delta.z = SPEED * forwardmovement.y * (dt / 1000.0);
			entity_move(pos, &delta);
		}
		if(keyboard[SDL_SCANCODE_A])
		{
			delta.x = SPEED * forwardmovement.y * (dt / 1000.0);
			delta.z = -SPEED * forwardmovement.x * (dt / 1000.0);
			entity_move(pos, &delta);
		}
		if(keyboard[SDL_SCANCODE_S])
		{
			delta.x = -SPEED * forwardmovement.x * (dt / 1000.0);
			delta.z = -SPEED * forwardmovement.y * (dt / 1000.0);
			entity_move(pos, &delta);
		}
		if(keyboard[SDL_SCANCODE_D])
		{
			delta.x = -SPEED * forwardmovement.y * (dt / 1000.0);
			delta.z = +SPEED * forwardmovement.x * (dt / 1000.0);
			entity_move(pos, &delta);
		}
		if(keyboard[SDL_SCANCODE_LSHIFT])
		{
			delta.x = 0;
			delta.y = -SPEED * (dt / 1000.0);
			delta.z = 0;
			entity_move(pos, &delta);
		}
		if(keyboard[SDL_SCANCODE_SPACE])
		{
			delta.x = 0;
			delta.y = SPEED * (dt / 1000.0);
			delta.z = 0;
			entity_move(pos, &delta);
		}
	}
	else
	{
		vec3_t forces = {0, 0, 0};
		if(keyboard[SDL_SCANCODE_W])
		{
			forces.x += FORCE * forwardmovement.x;
			forces.z += FORCE * forwardmovement.y;
		}
		if(keyboard[SDL_SCANCODE_A])
		{
			forces.x += FORCE * forwardmovement.y;
			forces.z += -FORCE * forwardmovement.x;
		}
		if(keyboard[SDL_SCANCODE_S])
		{
			forces.x += -SPEED * forwardmovement.x;
			forces.z += -SPEED * forwardmovement.y;
		}
		if(keyboard[SDL_SCANCODE_D])
		{
			forces.x += -FORCE * forwardmovement.y;
			forces.z += +FORCE * forwardmovement.x;
		}
		if(keyboard[SDL_SCANCODE_SPACE])
		{
			forces.y += 13;
		}
		entity_update(pos, &forces, dt/1000.0);
	}
	

	
	headpos = *posptr;
	headpos.y += 1.8;

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
				case SDLK_c:
					printf("Coords x: %f y: %f z: %f \n", posptr->x, posptr->y, posptr->z);
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
				game_rayadd(&headpos, &forwardcamera, b, 1);
			}
			else if(e.button.button == SDL_BUTTON_RIGHT)
			{
				game_raydel(&headpos, &forwardcamera);
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

	forwardcamera.x += headpos.x;
	forwardcamera.y += headpos.y;
	forwardcamera.z += headpos.z;
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
	mat4_t view = getviewmatrix(headpos, forwardcamera, up);

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
	world_render(*posptr);

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
