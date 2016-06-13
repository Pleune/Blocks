#include "state.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <SDL.h>
#include "gl.h"
#include "state.h"
#include "custommath.h"
#include "world.h"
#include "blockpick.h"
#include "debug.h"
#include "defines.h"
#include "entity.h"
#include "worldgen.h"
#include "textbox.h"

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049
#define GPU_MEMORY_INFO_EVICTED_MEMORY_NVX 0x904B

static int windoww = 0;
static int windowh = 0;

static GLuint drawprogram;
static GLuint viewprojectionmatrix;
static GLuint modelmatrix;

static GLuint ppprogram;
static GLuint pppointbuffer;
static GLuint postprocess_uniform_tex;
static GLuint postprocess_uniform_depth;
static GLuint postprocess_uniform_window_szie;

static textbox_t *textbox_fps;

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
const static vec3_t zero = {0,0,0};
const static vec3_t height = {0,PLAYER_EYEHEIGHT,0};

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
static int usecontroller = 1;

struct {
	double x, y;
} static leftstick = {0}, rightstick = {0};

static SDL_Thread *updatethread;
static SDL_sem *updatesem;
static int stopupdatethread;

void
update(uint32_t dt)
{
	if(!updating)
		return;
	long num = world_update_flush();
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

static inline double
deadzone(double d)
{
	if(d > 0)
		return d > .1 ? (d - .1) / .9 : 0;
	else
		return d < -.1 ? (d + .1) / .9 : 0;
}

void
state_game_init(void *ptr)
{
	state_window_get_size(&windoww, &windowh);

	//load shaders 'n stuff
	gl_program_load_file(&drawprogram, "shaders/vs", "shaders/fs");
	gl_program_load_file(&ppprogram, "shaders/pvs", "shaders/pfs");

	modelmatrix = glGetUniformLocation(drawprogram, "MODEL");
	viewprojectionmatrix = glGetUniformLocation(drawprogram, "VP");
	postprocess_uniform_tex = glGetUniformLocation(ppprogram, "tex");
	postprocess_uniform_depth = glGetUniformLocation(ppprogram, "depth");
	postprocess_uniform_window_szie = glGetUniformLocation(ppprogram, "window_size");

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
	world_seed_gen();
	vec3_t spawn = {0, 0, 0};

	spawn.y = worldgen_get_height_of_pos(0, 0, 0)+1.1;

	int spawntries = 0;
	while((spawn.y < 0 || spawn.y > 70) && spawntries < 500)
	{
		spawntries++;
		spawn.x = (double)(rand()%10000) - 5000;
		spawn.z = (double)(rand()%10000) - 5000;
		spawn.y = worldgen_get_height_of_pos(0, spawn.x, spawn.z)+1.1;
		printf("spawn retry %i x: %f z: %f h: %f\n", spawntries, spawn.x, spawn.z, spawn.y);
	}
	spawn.x += .5;
	spawn.z += .5;
	if(spawn.y < 0)
		spawn.y = 0.1;
	printf("h: %f\n", spawn.y);
	world_init(spawn);
	pos = entity_create(spawn.x, spawn.y, spawn.z, PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_MASS);
	posptr = entity_pos_get_ptr(pos);
	vec3_t friction = {PLAYER_FRICTION,PLAYER_FRICTION,PLAYER_FRICTION};
	entity_friction_set(pos, friction);
	state_mouse_center();
	SDL_ShowCursor(0);

	ticks = SDL_GetTicks();

	updatesem = SDL_CreateSemaphore(0);
	updatethread = SDL_CreateThread(updatethreadfunc, "updatethread", 0);

	textbox_fps = textbox_create(10, 10, 200, 100, "0fps", 0, TEXTBOX_FONT_ROBOTO_REGULAR, TEXTBOX_FONT_SIZE_MEDIUM, 0);
}

static void
input(uint32_t dt)
{
	SDL_PumpEvents();

	vec3_t inputvec = {0,0,0};

	const uint8_t *keyboard = SDL_GetKeyboardState(0);

	if(takeinput)
	{
		if(state_has_controller() && usecontroller)
		{
			inputvec.x = leftstick.x;
			inputvec.z = leftstick.y;
			rotx += JOYSTICK_SENSITIVITY_LOOK*rightstick.x*dt/1000.0;
			roty -= JOYSTICK_SENSITIVITY_LOOK*rightstick.y*dt/1000.0;
		} else {
			if(keyboard[SDL_SCANCODE_W])
				inputvec.z -= 1;
			if(keyboard[SDL_SCANCODE_A])
				inputvec.x -= 1;
			if(keyboard[SDL_SCANCODE_S])
				inputvec.z += 1;
			if(keyboard[SDL_SCANCODE_D])
				inputvec.x += 1;
			if(keyboard[SDL_SCANCODE_LSHIFT])
				inputvec.y -= 1;
			if(keyboard[SDL_SCANCODE_SPACE])
				inputvec.y += 1;

			int mousex, mousey;
			SDL_GetMouseState(&mousex, &mousey);
			state_mouse_center();
			double deltamousex = mousex - windoww/2;
			double deltamousey = mousey - windowh/2;

			rotx += MOUSE_SENSITIVITY*deltamousex;
			roty -= MOUSE_SENSITIVITY*deltamousey;

			roty = roty > M_PI/2-.005 ? M_PI/2-.005 : roty;
			roty = roty < -M_PI/2+.005 ? -M_PI/2+.005 : roty;

			rotx = rotx > M_PI*2 ? rotx - M_PI*2: rotx;
			rotx = rotx < -M_PI*2 ? rotx + M_PI*2: rotx;
		}
	}

	forwardcamera.x = sin(rotx) * cos(roty);
	forwardcamera.y = sin(roty);
	forwardcamera.z = -cos(rotx) * cos(roty);

	//vec2_t forwardmovement;
	//forwardmovement.x = sin(rotx);
	//forwardmovement.y = -cos(rotx);


	vec3_t rotatevec;
	rotatevec.x = cos(rotx)*inputvec.x - sin(rotx)*inputvec.z;
	rotatevec.z = sin(rotx)*inputvec.x + cos(rotx)*inputvec.z;
	rotatevec.y = inputvec.y;

	if(dt < 500)
	{
		if(flying)
		{
			rotatevec.x *= PLAYER_FLY_SPEED * dt / 1000.0;
			rotatevec.y *= PLAYER_FLY_SPEED * dt / 1000.0;
			rotatevec.z *= PLAYER_FLY_SPEED * dt / 1000.0;
			entity_move(pos, &rotatevec);
		} else {
			rotatevec.x *= PLAYER_WALK_MAX_FORCE;
			rotatevec.y = 0;
			rotatevec.z *= PLAYER_WALK_MAX_FORCE;
			entity_update(pos, &rotatevec, dt/1000.0);
		}
	}

	if(keyboard[SDL_SCANCODE_R])
	{
		block_t b;
		b.id = WATER;
		b.metadata.number = SIM_WATER_LEVELS;
		world_ray_set(&headpos, &forwardcamera, b, 1, 1, 1000);
	}
	if(keyboard[SDL_SCANCODE_E])
	{
		world_ray_del(&headpos, &forwardcamera, 1, 1000);
	}
	if(keyboard[SDL_SCANCODE_C])
	{
		vec3_t dir;
		for(dir.x = -1; dir.x < 1; dir.x += .3)
		for(dir.y = -1; dir.y < 0; dir.y += .3)
		for(dir.z = -1; dir.z < 1; dir.z += .3)
			world_ray_del(&headpos, &dir, 1, 50);
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
				state_queue_pop();
			break;
			case SDLK_SPACE:
				entity_jump(pos, PLAYER_JUMPSPEED);
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
				top.y = worldgen_get_height_of_pos(0, posptr->x, posptr->z)+1;
				entity_pos_set(pos, top);
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

	else if(e.type == SDL_CONTROLLERBUTTONDOWN)
	{
		switch(e.cbutton.button)
		{
			case SDL_CONTROLLER_BUTTON_A:
				entity_jump(pos, PLAYER_JUMPSPEED);
				break;
		}
	}

	else if(e.type == SDL_CONTROLLERAXISMOTION)
	{
		switch(e.caxis.axis)
		{
			case SDL_CONTROLLER_AXIS_LEFTX:
				leftstick.x = deadzone(e.caxis.value / 32768.0);
				break;
			case SDL_CONTROLLER_AXIS_LEFTY:
				leftstick.y = deadzone(e.caxis.value / 32768.0);
				break;
			case SDL_CONTROLLER_AXIS_RIGHTX:
				rightstick.x = deadzone(e.caxis.value / 32768.0);
				break;
			case SDL_CONTROLLER_AXIS_RIGHTY:
				rightstick.y = deadzone(e.caxis.value / 32768.0);
				break;
		}
	}

	else if(e.type == SDL_MOUSEBUTTONDOWN)
	{
		if(e.button.button == SDL_BUTTON_LEFT)
		{
			block_t b;
			b.id = WATER_GEN;
			world_ray_set(&headpos, &forwardcamera, b, 1, 1, 1000);
		}
		else if(e.button.button == SDL_BUTTON_RIGHT)
		{
			world_ray_del(&headpos, &forwardcamera, 1, 1000);
		}
	}
	else if(e.type == SDL_WINDOWEVENT)
	{
		if(e.window.event == SDL_WINDOWEVENT_RESIZED)
		{
			state_window_get_size(&windoww, &windowh);

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
		static char buffer[32];

		snprintf(buffer, 32, "%ifps", frame);
		textbox_set_txt(textbox_fps, buffer);

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
		printf("%li triangles\n", world_get_trianglecount());

		frame=0;
	}

	vec3_t forward = {
		forwardcamera.x,
		forwardcamera.y + PLAYER_EYEHEIGHT,
		forwardcamera.z,
	};

	mat4_t projection = getprojectionmatrix(90, (float)windoww / (float)windowh, 3000, .1);
	mat4_t view = getviewmatrix(height, forward, up);

	mat4_t vp;
	dotmat4mat4(&vp, &projection, &view);

	//render to framebuffer here
	if(pp)
		glBindFramebuffer(GL_FRAMEBUFFER, renderbuffer.framebuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(lines)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glUseProgram(drawprogram);
	glUniformMatrix4fv(viewprojectionmatrix, 1, GL_FALSE, vp.mat);
	world_render(*posptr, modelmatrix);

	if(lines)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//render to screen here
	if(pp)
	{
		glUseProgram(ppprogram);

		GLfloat window_vec[2] = {windoww, windowh};
		glUniform2fv(postprocess_uniform_window_szie, 1, window_vec);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindBuffer(GL_ARRAY_BUFFER, pppointbuffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderbuffer.colorbuffer);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, renderbuffer.depthbuffer);

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	textbox_render(textbox_fps);

	state_window_swap();
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

	input(dt);
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
	textbox_destroy(textbox_fps);
}
