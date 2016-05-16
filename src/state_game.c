#include "state.h"
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

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049 
#define GPU_MEMORY_INFO_EVICTED_MEMORY_NVX 0x904B

static int windoww = 0;
int windowh = 0;

static GLuint drawprogram;
static GLuint drawprogrammatrixinput;

static GLuint ppprogram;
static GLuint pppointbuffer;
static GLuint pppointbufferid;

struct {
	GLuint framebuffer;

	GLuint colorbuffer;
	GLuint depthbuffer;

	GLuint colorbufferid;
	GLuint depthbufferid;
} static renderbuffer;

static vec3_t forwardcamera;
static vec3_t headpos;

static uint32_t ticks = 0;

const static vec3_t up = {0,1,0};

static int fpscap = 0;
const static int fpsmax = 120;

static entity_t *pos;
const static vec3_t *posptr;
static float rotx, roty;

static int lines = 0;
static int pp = 1;
static int takeinput = 1;
static int flying = 0;
static int updating = 1;

static SDL_Thread *updatethread;
static SDL_sem *updatesem;
static int stopupdatethread;

void
update(uint32_t dt)
{
	if(!updating)
		return;
	long num = world_updaterun();
	if(num)
		printf("u:%li\n", num);
}

static int
updatethreadfunc(void *ptr)
{
	stopupdatethread = 0;
	while(1)
	{
		SDL_SemWait(updatesem);
		if(stopupdatethread)
			break;
		update(20);
	}
	return 0;

}

void
state_game_init(void *ptr)
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
	world_genseed();
	vec3_t spawn = {0, 0, 0};

	spawn.y = worldgen_getheightfrompos(0, 0, 0)+1.1;

	int spawntries = 0;
	while((spawn.y < 0 || spawn.y > 70) && spawntries < 500)
	{
		spawntries++;
		spawn.x = (double)(rand()%10000) - 5000;
		spawn.z = (double)(rand()%10000) - 5000;
		spawn.y = worldgen_getheightfrompos(0, spawn.x, spawn.z)+1.1;
		printf("spawn retry %i x: %f z: %f h: %f\n", spawntries, spawn.x, spawn.z, spawn.y);
	}
	spawn.x += .5;
	spawn.z += .5;
	if(spawn.y < 0)
		spawn.y = 0.1;
	printf("h: %f\n", spawn.y);
	world_init(spawn);
	pos = entity_create(spawn.x, spawn.y, spawn.z, PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_MASS);
	posptr = entity_getposptr(pos);
	vec3_t friction = {PLAYER_FRICTION,PLAYER_FRICTION,PLAYER_FRICTION};
	entity_setfriction(pos, friction);
	centermouse();
	SDL_ShowCursor(0);

	ticks = SDL_GetTicks();

	updatesem = SDL_CreateSemaphore(0);
	updatethread = SDL_CreateThread(updatethreadfunc, "updatethread", 0);
}

static void
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

		rotx += MOUSE_SENSITIVITY*deltamousex;
		roty -= MOUSE_SENSITIVITY*deltamousey;

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
			forces.x += -FORCE * forwardmovement.x;
			forces.z += -FORCE * forwardmovement.y;
		}
		if(keyboard[SDL_SCANCODE_D])
		{
			forces.x += -FORCE * forwardmovement.y;
			forces.z += +FORCE * forwardmovement.x;
		}
		if(dt < 500)
			entity_update(pos, &forces, dt/1000.0);
	}

	if(keyboard[SDL_SCANCODE_R])
	{
		block_t b;
		b.id = WATER;
		b.metadata.number = SIM_WATER_LEVELS;
		world_rayadd(&headpos, &forwardcamera, b, 1, 1, 1000);
	}
	if(keyboard[SDL_SCANCODE_E])
	{
		world_raydel(&headpos, &forwardcamera, 1, 1000);
	}
	if(keyboard[SDL_SCANCODE_C])
	{
		vec3_t dir;
		for(dir.x = -1; dir.x < 1; dir.x += .3)
		for(dir.y = -1; dir.y < 0; dir.y += .3)
		for(dir.z = -1; dir.z < 1; dir.z += .3)
			world_raydel(&headpos, &dir, 1, 50);
	}


	headpos = *posptr;
	headpos.y += PLAYER_EYEHEIGHT;
}

void
state_game_event(void *ptr)
{
	SDL_Event e = ((SDL_Event *)ptr)[0];

	if(e.type == SDL_KEYDOWN)
	{
		switch(e.key.keysym.sym)
		{
			case SDLK_ESCAPE:
				state_queuepop();
			break;
			case SDLK_SPACE:
				entity_jump(pos, JUMPSPEED);
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
			case SDLK_t:
			{
				vec3_t top = *posptr;
				top.y = worldgen_getheightfrompos(0, posptr->x, posptr->z)+1;
				entity_setpos(pos, top);
			break;
			}
			case SDLK_f:
				flying = !flying;
			break;
			case SDLK_u:
				updating = !updating;
				printf("UPDATING: %i\n", updating);
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
			b.id = WATER_GEN;
			world_rayadd(&headpos, &forwardcamera, b, 1, 1, 1000);
		}
		else if(e.button.button == SDL_BUTTON_RIGHT)
		{
			world_raydel(&headpos, &forwardcamera, 1, 1000);
		}
	}
	else if(e.type == SDL_WINDOWEVENT)
	{
		if(e.window.event == SDL_WINDOWEVENT_RESIZED)
		{
			getwindowsize(&windoww, &windowh);

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

static void
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

		GLint total_mem_kb = 0;
		glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, 
		&total_mem_kb);
 
		GLint cur_avail_mem_kb = 0;
		glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, 
		&cur_avail_mem_kb);
 
		GLint cur_evicted_mem_kb = 0;
		glGetIntegerv(GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, 
		&cur_evicted_mem_kb);

		printf("MEM AVAIL: %i\t /%imb\n", cur_avail_mem_kb/1000, total_mem_kb/1000);
		printf("           %imb evicted\n", cur_evicted_mem_kb/1000);
		printf("%li triangles\n", world_gettrianglecount());

		frame=0;
	}

	input(dt);

	vec3_t forwardview;
	forwardview.x = forwardcamera.x + headpos.x;
	forwardview.y = forwardcamera.y + headpos.y;
	forwardview.z = forwardcamera.z + headpos.z;

	mat4_t projection = getprojectionmatrix(90, (float)windoww / (float)windowh, 3000, .1);
	mat4_t view = getviewmatrix(headpos, forwardview, up);

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
state_game_run(void *ptr)
{
	uint32_t newticks = SDL_GetTicks();
	uint32_t dt = ticks ? newticks - ticks : 0;
	ticks = newticks;

	static uint32_t updatebuild = 0;
	updatebuild += dt;
	if(updatebuild >= 20)
	{
		updatebuild -= 20;
		SDL_SemTryWait(updatesem);
		SDL_SemPost(updatesem);
	}
	render(dt);

	if(fpscap)
	{
		uint32_t ticksdiff = SDL_GetTicks() - ticks;
		if(ticksdiff < 1000 / fpsmax)
			SDL_Delay(1000 / fpsmax - ticksdiff);
	}
}

void
state_game_close(void *ptr)
{
	glDeleteProgram(drawprogram);
	glDeleteFramebuffers(1, &renderbuffer.framebuffer);
	glDeleteTextures(1, &renderbuffer.colorbuffer);
	glDeleteTextures(1, &renderbuffer.depthbuffer);
	stopupdatethread = 1;
	SDL_SemPost(updatesem);
	SDL_WaitThread(updatethread, 0);
	SDL_DestroySemaphore(updatesem);
	entity_destroy(pos);
	world_cleanup();
}
