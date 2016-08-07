#include <stdlib.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <GL/glew.h>

#include "state.h"
#include "textbox.h"
#include "debug.h"
#include "defines.h"
#include "hmap.h"
#include "save.h"

static int isrunning = 1;

#define STACK_MAX 255
struct statestack_s {
	enum states states[STACK_MAX];
	int instances[MAX_STATES];
	int top;
};

static struct statestack_s stack = { {0}, {0}, 0 };
#define CURRENTSTATE (stack.states[stack.top])

static int queueforpop = 0;
static int queueforfail = 0;
static enum states queueforpush = MAX_STATES;
static void *pushptr = 0;

static SDL_Window *win;
static SDL_GLContext glcontext;
static int windoww, windowh;
static char *basepath;
static char *prefpath;

static SDL_GameController *controller = NULL;

static void
runevent(enum states s, enum events e, void *ptr)
{
	if(statetable[s][e])
		(*statetable[s][e]) (ptr);
}

static void
push(enum states newstate)
{
	info("state engine: push state %i onto %i", newstate, CURRENTSTATE);

	runevent(CURRENTSTATE, PAUSE, pushptr);

	if(stack.instances[newstate] == 0)
		runevent(newstate, INITALIZE, pushptr);
	else
		runevent(newstate, RESUME, pushptr);

	stack.instances[newstate]++;
	stack.top++;
	if(stack.top < STACK_MAX)
		stack.states[stack.top] = newstate;
	else
		fail("state stack full");
}

static void
failedinit()
{
	info("state engine: fail state %i", CURRENTSTATE);

	stack.instances[CURRENTSTATE]--;

	stack.top--;
	if(stack.top >= 0)
		stack.states[stack.top + 1] = 0;
	else
		isrunning = 0;

	runevent(CURRENTSTATE, RESUME, 0);
}

static void
pop()
{
	info("state engine: pop state %i", CURRENTSTATE);

	stack.instances[CURRENTSTATE]--;

	if(stack.instances[CURRENTSTATE] == 0)
		runevent(CURRENTSTATE, CLOSE, 0);
	else
		runevent(CURRENTSTATE, PAUSE, 0);

	stack.top--;
	if(stack.top >= 0)
	{
		stack.states[stack.top + 1] = 0;
	} else {
		isrunning = 0;
		return;
	}

	runevent(CURRENTSTATE, RESUME, 0);
}

static void
cleanup()
{
	info("state engine: cleaning up for either an exit or a reboot");

	textbox_static_cleanup();
	TTF_Quit();
	SDL_GameControllerClose(controller);
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(win);
	SDL_Quit();
	free(basepath);
}

static void
init()
{
	info("state engine: initalizing the program");

	if(SDL_Init(SDL_INIT_EVERYTHING))
		//SDL2 init_chunk failed
		fail("SDL_Init(SDL_INIT_EVERYTHING)");

	win = SDL_CreateWindow("Blocks", 100, 100, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if(!win)
		//SDL2 didn't create a window
		fail("SDL_CreateWindow");

	int i;
	for (i = 0; i < SDL_NumJoysticks(); ++i) {
		if(SDL_IsGameController(i))
		{
			controller = SDL_GameControllerOpen(i);
			if(controller)
			{
				info("Game controller found.");
				break;
			} else {
				warn("Could not open gamecontroller %i: %s", i, SDL_GetError());
			}
		}
	}

	glcontext = SDL_GL_CreateContext(win);
	if(!glcontext)
		//SDL2 didint create the required link with opengl
		fail("SDL_GL_CreateContext()");

	if(glewInit())
		//glew couldn't do to wrangling
		fail("glewInit()");

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	SDL_GetWindowSize(win, &windoww, &windowh);
	SDL_GL_SetSwapInterval(0);

	glClearColor(0,0,0,1);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(win);

	basepath = SDL_GetBasePath();
	prefpath = SDL_GetPrefPath(PROGRAM_ORG, PROGRAM_NAME);

	stack.instances[MENUMAIN] = 1;
	stack.top = 0;

	TTF_Init();
	textbox_static_init();

	runevent(MENUMAIN, INITALIZE, 0);
}

int
main(int argc, char *argv[])
{
	init();
	while(isrunning)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT)
			{
				state_exit();
			} else if(e.type == SDL_WINDOWEVENT)
			{
				if(e.window.event == SDL_WINDOWEVENT_RESIZED)
					state_window_update(e.window.data1, e.window.data2);
			}
			runevent(CURRENTSTATE, SDLEVENT, &e);
		}
		runevent(CURRENTSTATE, RUN, 0);

		if(queueforpop)
		{
			queueforpop = 0;
			pop();
		}
		if(queueforpush != MAX_STATES)
		{
			enum states state = queueforpush;
			queueforpush = MAX_STATES;
			push(state);
		}
		if(queueforfail)
		{
			queueforfail = 0;
			failedinit();
		}
	}

	while(stack.top > 0)
		pop();

	cleanup();

	return 0;
}

void
state_queue_push(enum states state, void *ptr)
{
	if(queueforpush == MAX_STATES)
	{
		pushptr = ptr;
		queueforpush = state;
	} else {
		error("Too manu state_queue_push()");
	}
}

void
state_queue_pop()
{
	if(!queueforpop)
		queueforpop = 1;
	else
		error("Too many state_queue_pop()");
}

void
state_queue_fail()
{
	if(!queueforfail)
		queueforfail = 1;
	else
		error("Too many state_queue_fail()");

	queueforpop = 0;
}

int
state_is_initalized(enum states state)
{
	return stack.instances[state];
}

void
state_exit()
{
	//cleanup before closing.
	info("Exiting the program");
	isrunning = 0;
}

void
state_window_update(int w, int h)
{
	glViewport(0, 0, w, h);
	windoww = w;
	windowh = h;
}

void
state_window_get_size(int *w, int*h)
{
	*w = windoww;
	*h = windowh;
}

void
state_window_swap()
{
	SDL_GL_SwapWindow(win);
}

uint32_t
state_window_get_id()
{
	return SDL_GetWindowID(win);
}

const char *
state_basepath_get()
{
	return basepath;
}

const char *
state_prefpath_get()
{
	return prefpath;
}

void
state_mouse_center()
{
	SDL_WarpMouseInWindow(win, windoww/2, windowh/2);
}

int
state_has_controller()
{
	return controller ? 1 : 0;
}
